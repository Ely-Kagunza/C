#define _CRT_SECURE_NO_WARNINGS
#include "perf_optimizer.h"
#include <stdlib.h>
#include <math.h>

// ============== PERFORMANCE MEASUREMENT ==============

Timer timer_start(void) {
    Timer t;
    QueryPerformanceFrequency(&t.frequency);
    QueryPerformanceCounter(&t.start);
    return t;
}

void timer_stop(Timer *t) {
    QueryPerformanceCounter(&t->end);
}

double timer_elapsed_ms(Timer t) {
    return ((double)(t.end.QuadPart - t.start.QuadPart) / 
            (double)t.frequency.QuadPart) * 1000.0;
}

// ============== AVX2 DOT PRODUCT ==============

// Non-vectorized baseline (for comparison)
static float dot_product_scalar(const float *a, const float *b, int n) {
    float result = 0.0f;
    for (int i = 0; i < n; i++) {
        result += a[i] * b[i];
    }
    return result;
}

// AVX2 vectorized: Process 8 floats per iteration
float simd_dot_product(const float *a, const float *b, int n) {
    __m256 sum = _mm256_setzero_ps();  // 8x 0.0
    
    // Process 8 floats at a time
    int i = 0;
    for (; i <= n - 8; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);  // Load 8 floats from a
        __m256 vb = _mm256_loadu_ps(&b[i]);  // Load 8 floats from b
        __m256 prod = _mm256_mul_ps(va, vb); // Multiply element-wise
        sum = _mm256_add_ps(sum, prod);      // Accumulate
    }
    
    // Vertical sum: Reduce 8 floats to 1
    // shuffle: [a,b,c,d,e,f,g,h] -> add pairs -> [a+e, b+f, c+g, d+h, ...]
    __m128 sum128 = _mm256_castps256_ps128(sum);
    __m128 hi128 = _mm256_extractf128_ps(sum, 1);
    sum128 = _mm_add_ps(sum128, hi128);
    
    // Horizontal add across 128-bit lane
    __m128 shuf = _mm_shuffle_ps(sum128, sum128, _MM_SHUFFLE(2, 3, 0, 1));
    sum128 = _mm_add_ps(sum128, shuf);
    shuf = _mm_shuffle_ps(sum128, sum128, _MM_SHUFFLE(1, 0, 3, 2));
    sum128 = _mm_add_ps(sum128, shuf);
    
    float result = _mm_cvtss_f32(sum128);
    
    // Handle remaining elements (n % 8)
    for (; i < n; i++) {
        result += a[i] * b[i];
    }
    
    return result;
}

// ============== AVX2 MATRIX-VECTOR MULTIPLY ==============

void simd_matrix_vector_multiply(
    const float *A, int m, int n,
    const float *x,
    float *y
) {
    // y[i] = A[i,:] · x = sum(A[i][j] * x[j])
    // Process 4 rows at a time for cache efficiency
    
    for (int i = 0; i < m; i++) {
        y[i] = simd_dot_product(&A[i * n], x, n);
    }
}

// ============== BATCH MATRIX-VECTOR MULTIPLY ==============

void simd_batch_matrix_vector_multiply(
    const float *A, int m, int n,
    const float *x_batch, int batch_size,
    float *y_batch
) {
    // Process batch_size requests in parallel
    // Each request: y[k] = A * x_batch[k]
    
    for (int k = 0; k < batch_size; k++) {
        const float *x = &x_batch[k * n];
        float *y = &y_batch[k * m];
        simd_matrix_vector_multiply(A, m, n, x, y);
    }
}

// ============== NEURAL NETWORK FORWARD PASS ==============

NeuralNetwork* nn_create(void) {
    NeuralNetwork *nn = malloc(sizeof(NeuralNetwork));
    
    // Layer 1: 784 inputs -> 128 hidden units
    nn->W1 = malloc(128 * 784 * sizeof(float));
    nn->b1 = malloc(128 * sizeof(float));
    
    // Layer 2: 128 hidden -> 10 output
    nn->W2 = malloc(10 * 128 * sizeof(float));
    nn->b2 = malloc(10 * sizeof(float));
    
    // Temporary buffers
    nn->hidden = malloc(128 * sizeof(float));
    nn->output = malloc(10 * sizeof(float));
    
    // Initialize weights with random small values
    srand(12345);
    for (int i = 0; i < 128 * 784; i++) {
        nn->W1[i] = (float)rand() / RAND_MAX * 0.01f;
    }
    for (int i = 0; i < 128; i++) {
        nn->b1[i] = 0.0f;
    }
    for (int i = 0; i < 10 * 128; i++) {
        nn->W2[i] = (float)rand() / RAND_MAX * 0.01f;
    }
    for (int i = 0; i < 10; i++) {
        nn->b2[i] = 0.0f;
    }
    
    return nn;
}

void nn_forward(NeuralNetwork *nn, const float *input) {
    // Layer 1: hidden = ReLU(W1 @ input + b1)
    simd_matrix_vector_multiply(nn->W1, 128, 784, input, nn->hidden);
    
    // Add bias and ReLU
    for (int i = 0; i < 128; i++) {
        nn->hidden[i] += nn->b1[i];
        if (nn->hidden[i] < 0.0f) nn->hidden[i] = 0.0f;  // ReLU
    }
    
    // Layer 2: output = W2 @ hidden + b2
    simd_matrix_vector_multiply(nn->W2, 10, 128, nn->hidden, nn->output);
    
    // Add bias
    for (int i = 0; i < 10; i++) {
        nn->output[i] += nn->b2[i];
    }
}

void nn_free(NeuralNetwork *nn) {
    if (!nn) return;
    free(nn->W1);
    free(nn->b1);
    free(nn->W2);
    free(nn->b2);
    free(nn->hidden);
    free(nn->output);
    free(nn);
}

// ============== BATCH PROCESSING ==============

InferenceBatch* batch_create(void) {
    InferenceBatch *b = malloc(sizeof(InferenceBatch));
    b->count = 0;
    return b;
}

void batch_add_request(InferenceBatch *batch, float *input, float *output) {
    if (batch->count >= MAX_BATCH_SIZE) {
        fprintf(stderr, "Batch full! Process before adding more.\n");
        return;
    }
    batch->request_inputs[batch->count] = input;
    batch->response_outputs[batch->count] = output;
    batch->count++;
}

int batch_is_full(InferenceBatch *batch) {
    return batch->count >= MAX_BATCH_SIZE;
}

void batch_process(InferenceBatch *batch, NeuralNetwork *nn) {
    // Process all requests in the batch
    for (int i = 0; i < batch->count; i++) {
        nn_forward(nn, batch->request_inputs[i]);
        memcpy(batch->response_outputs[i], nn->output, 10 * sizeof(float));
    }
}

void batch_reset(InferenceBatch *batch) {
    batch->count = 0;
}

void batch_free(InferenceBatch *batch) {
    if (batch) free(batch);
}

// ============== METRICS ==============

void metrics_record(PerformanceMetrics *m, double inference_time_ms) {
    m->total_inferences++;
    m->total_inference_time_ms += inference_time_ms;
    
    if (m->total_inferences == 1) {
        m->min_inference_time_ms = inference_time_ms;
        m->max_inference_time_ms = inference_time_ms;
    } else {
        if (inference_time_ms < m->min_inference_time_ms) {
            m->min_inference_time_ms = inference_time_ms;
        }
        if (inference_time_ms > m->max_inference_time_ms) {
            m->max_inference_time_ms = inference_time_ms;
        }
    }
    
    m->avg_inference_time_ms = m->total_inference_time_ms / m->total_inferences;
}

void metrics_display(PerformanceMetrics *m) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║           Performance Metrics (AVX2 Optimized)           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    printf("Total inferences:        %llu\n", m->total_inferences);
    printf("Total time:              %.2f ms\n", m->total_inference_time_ms);
    printf("Average latency:         %.4f ms/request\n", m->avg_inference_time_ms);
    printf("Min latency:             %.4f ms\n", m->min_inference_time_ms);
    printf("Max latency:             %.4f ms\n", m->max_inference_time_ms);
    printf("Throughput:              %.0f req/sec\n", 
           1000.0 / m->avg_inference_time_ms);
    printf("\n");
}