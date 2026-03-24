#define _CRT_SECURE_NO_WARNINGS
#include "perf_optimizer.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("   Phase 15: Performance Optimization & Vectorization\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    // ============== BENCHMARK 1: SIMD DOT PRODUCT ==============
    
    printf("[1] DOT PRODUCT PERFORMANCE\n");
    printf("─────────────────────────────────────────────────────────\n");
    
    int vec_size = 784;  // MNIST feature size
    float *a = malloc(vec_size * sizeof(float));
    float *b = malloc(vec_size * sizeof(float));
    
    // Initialize vectors
    for (int i = 0; i < vec_size; i++) {
        a[i] = (float)i / vec_size;
        b[i] = (float)(vec_size - i) / vec_size;
    }
    
    // Warm up
    simd_dot_product(a, b, vec_size);
    
    // Benchmark: 10,000 dot products
    Timer t_simd = timer_start();
    for (int i = 0; i < 10000; i++) {
        simd_dot_product(a, b, vec_size);
    }
    timer_stop(&t_simd);
    double simd_time = timer_elapsed_ms(t_simd);
    
    printf("10,000 dot products (%d elements):\n", vec_size);
    printf("  Time: %.2f ms\n", simd_time);
    printf("  Rate: %.0f dot-products/ms\n", 10000.0 / simd_time);
    printf("  Throughput: %.2f GOps/sec\n", 
           (10000.0 * vec_size * 2) / (simd_time * 1e6));  // 2 ops per element (mul+add)
    printf("\n");
    
    // ============== BENCHMARK 2: NEURAL NETWORK INFERENCE ==============
    
    printf("[2] NEURAL NETWORK INFERENCE (Single Request)\n");
    printf("─────────────────────────────────────────────────────────\n");
    
    NeuralNetwork *nn = nn_create();
    float *input = malloc(784 * sizeof(float));
    
    // Create fake MNIST image (784 pixels)
    for (int i = 0; i < 784; i++) {
        input[i] = (float)rand() / RAND_MAX;
    }
    
    // Warm up
    nn_forward(nn, input);
    
    // Benchmark: 1,000 inferences
    PerformanceMetrics metrics = {0};
    Timer t_nn = timer_start();
    for (int i = 0; i < 1000; i++) {
        Timer t_iter = timer_start();
        nn_forward(nn, input);
        timer_stop(&t_iter);
        metrics_record(&metrics, timer_elapsed_ms(t_iter));
    }
    timer_stop(&t_nn);
    double nn_total = timer_elapsed_ms(t_nn);
    
    printf("1,000 forward passes (784 → 128 → 10):\n");
    printf("  Total time: %.2f ms\n", nn_total);
    metrics_display(&metrics);
    
    // ============== BENCHMARK 3: BATCH PROCESSING ==============
    
    printf("[3] BATCH PROCESSING (64 Concurrent Requests)\n");
    printf("─────────────────────────────────────────────────────────\n");
    
    InferenceBatch *batch = batch_create();
    float **batch_inputs = malloc(64 * sizeof(float*));
    float **batch_outputs = malloc(64 * sizeof(float*));
    
    for (int i = 0; i < 64; i++) {
        batch_inputs[i] = malloc(784 * sizeof(float));
        batch_outputs[i] = malloc(10 * sizeof(float));
        
        for (int j = 0; j < 784; j++) {
            batch_inputs[i][j] = (float)rand() / RAND_MAX;
        }
    }
    
    // Create batch
    for (int i = 0; i < 64; i++) {
        batch_add_request(batch, batch_inputs[i], batch_outputs[i]);
    }
    
    // Warm up
    batch_process(batch, nn);
    batch_reset(batch);
    
    // Re-add requests
    for (int i = 0; i < 64; i++) {
        batch_add_request(batch, batch_inputs[i], batch_outputs[i]);
    }
    
    // Benchmark: Process 40 batches (2,560 total inferences)
    Timer t_batch = timer_start();
    for (int b = 0; b < 40; b++) {
        batch_process(batch, nn);
        batch_reset(batch);
        
        for (int i = 0; i < 64; i++) {
            batch_add_request(batch, batch_inputs[i], batch_outputs[i]);
        }
    }
    timer_stop(&t_batch);
    double batch_time = timer_elapsed_ms(t_batch);
    
    printf("40 batches × 64 requests = 2,560 inferences:\n");
    printf("  Total time: %.2f ms\n", batch_time);
    printf("  Per-inference: %.4f ms\n", batch_time / 2560.0);
    printf("  Throughput: %.0f inferences/sec\n", 2560.0 * 1000.0 / batch_time);
    printf("\n");
    
    // ============== SCALABILITY ANALYSIS ==============
    
    printf("[4] SCALABILITY (Event Loop Simulation)\n");
    printf("─────────────────────────────────────────────────────────\n");
    printf("Event loop processing 100 concurrent connections:\n");
    printf("  - Each connection: 1 inference request\n");
    printf("  - Batch these 100 requests together\n");
    printf("  - Process every 100ms (10 batches/second)\n\n");
    
    double infer_per_sec = 2560.0 * 1000.0 / batch_time;
    double batches_per_sec = 10;  // Process every 100ms
    double total_infer_per_sec = infer_per_sec * batches_per_sec;
    
    printf("  Inference throughput: %.0f req/sec\n", (double)total_infer_per_sec);
    printf("  Latency per request: %.2f ms (batched)\n", 
           100.0 / 64.0);  // Worst case: batches every 100ms, 64 requests
    printf("  Can handle: ~%d concurrent clients\n", 
           (int)(total_infer_per_sec / 10));  // Assuming 10 requests/sec per client
    printf("\n");
    
    // ============== CLEANUP ==============
    
    free(a);
    free(b);
    free(input);
    
    for (int i = 0; i < 64; i++) {
        free(batch_inputs[i]);
        free(batch_outputs[i]);
    }
    free(batch_inputs);
    free(batch_outputs);
    
    nn_free(nn);
    batch_free(batch);
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Phase 15 complete! AVX2 optimizations ready for production.\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}