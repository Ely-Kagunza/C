#ifndef PERF_OPTIMIZER_H
#define PERF_OPTIMIZER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>  // AVX2 intrinsics
#include <windows.h>
#include <time.h>

// =====================  PERFORMANCE MEASUREMENT =====================

typedef struct {
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
} Timer;

Timer timer_start(void);
void timer_stop(Timer *t);
double timer_elapsed_ms(Timer t);

// ===================== SIMD UTILITIES (AVX2) =====================

// Dot product: (a · b) for vectors of floats
// Non-vectorized: 1 result per 4 cycles
// AVX2 vectorized: 4 results per 4 cycles (4x speedup)
float simd_dot_product(const float *a, const float *b, int n);

// Matrix-vector multiply: y = A * x
// A is (m x n), x is (n,), result y is (m,)
// AVX2: Process 4 rows at a time
void simd_matrix_vector_multiply(
    const float *A, int m, int n,
    const float *x,
    float *y
);

// Batch matrix multiply: Process multiple vectors at once
// Useful for processing 64 images concurrently
// C[i] = A * x[i] for i in [0, batche_size]
void simd_batch_matrix_vector_multiply(
    const float *A, int m, int n,
    const float *x_batch, int batch_size,
    float *y_batch
);

// ===================== INFERENCE SIMULATION =====================

// Lightweight neural network: 2-layer fully connected
// Layer 1: input (784) → hidden (128)
// Layer 2: hidden (128) → output (10) for MNIST-style classification
typedef struct {
    float *W1; // 128 x 784 weight matrix
    float *b1; // 128 bias vector
    float *W2; // 10 x 128 weight matrix
    float *b2; // 10 bias vector
    float *hidden; // 128 temporary buffer
    float *output; // 10 output buffer
} NeuralNetwork;

NeuralNetwork *nn_create(void);
void nn_forward(NeuralNetwork *nn, const float *input);
void nn_free(NeuralNetwork *nn);

// ===================== BATCH PROCESSING =====================

#define MAX_BATCH_SIZE 64

typedef struct {
    float *request_inputs[MAX_BATCH_SIZE];   // Pointers to input vectors (784 floats each)
    float *response_outputs[MAX_BATCH_SIZE]; // Pointers to output vectors (10 floats each)
    int count;
} InferenceBatch;

InferenceBatch* batch_create(void);
void batch_add_request(InferenceBatch *batch, float *input, float *output);
int batch_is_full(InferenceBatch *batch);
void batch_process(InferenceBatch *batch, NeuralNetwork *nn);
void batch_free(InferenceBatch *batch);
void batch_reset(InferenceBatch *batch);

// ===================== METRICS =====================

typedef struct {
    uint64_t total_inferences;
    double total_inference_time_ms;
    double min_inference_time_ms;
    double max_inference_time_ms;
    double avg_inference_time_ms;
} PerformanceMetrics;

void metrics_record(PerformanceMetrics *m, double inference_time_ms);
void metrics_display(PerformanceMetrics *m);

#endif // PERF_OPTIMIZER_H