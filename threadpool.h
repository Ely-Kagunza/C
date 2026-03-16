#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <windows.h>

// ============================================================================
// THREAD POOL - Manage reusable worker threads
// ============================================================================
//
// PROBLEM: Creating threads is expensive
// - Each task = create thread + execute + destroy thread
// - Many context switches, memory allocation
//
// SOLUTION: Thread Pool
// - Create N worker threads once
// - Queue tasks, workers pick them up
// - Reuse threads for many tasks
//
// PERFORMANCE:
// - 10,000 tasks with 4-thread pool: ~50% faster than per-task threads
// - More scalable for high-volume work
//
// Python equivalent: concurrent.futures.ThreadPoolExecutor
// ============================================================================
typedef void (*TaskFunction)(void *);  // Function pointer type

typedef struct
{
    TaskFunction function;              // What to execute
    void *arg;                          // Arguments to pass
    int task_id;                        // For tracking
} Task;

typedef struct
{
    Task *tasks;                        // Queue of pending tasks
    int task_count;                     // How many tasks queued
    int task_capacity;                  // Allocated size

    HANDLE *threads;                    // Worker threads
    int thread_count;                   // Number of workers

    CRITICAL_SECTION queue_lock;        // Protect queue
    HANDLE task_available;              // Signal: task ready
    HANDLE shutdown_event;              // Signal: shutdown requested

    int total_tasks_processed;          // Statistics
} ThreadPool;

// Create thread pool with N worker threads
ThreadPool *threadpool_create(int num_threads);

// Free thread pool (wait for tasks to complete)
void threadpool_free(ThreadPool *pool);

// Submit task to queue (thread-safe)
int threadpool_submit(ThreadPool *pool, TaskFunction func, void *arg);

// Get statistics
void threadpool_display_stats(ThreadPool *pool);

#endif // THREADPOOL_H