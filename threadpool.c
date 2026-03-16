#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "threadpool.h"

// ============================================================================
// WORKER THREAD - Loop: wait for task, execute, repeat
// ============================================================================
//
// ALGORITHM:
// 1. Wait for task_available signal
// 2. Lock queue
// 3. If task, get it from queue
// 4. Unlock queue
// 5. Execute task
// 6. Return to step 1
// ============================================================================
static DWORD WINAPI worker_thread(LPVOID arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    while (1)
    {
        // Wait for either task available or shutdown
        HANDLE events[2] = {pool->task_available, pool->shutdown_event};
        DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (result == WAIT_OBJECT_0 + 1)
        {
            // Shutdown event triggered
            printf("[Worker] Shutting down\n");
            break;
        }

        // Task available - get it from queue
        EnterCriticalSection(&pool->queue_lock);

        if (pool->task_count > 0)
        {
            // Get first task
            Task task = pool->tasks[0];

            // Shift remaining tasks forward
            for (int i = 0; i < pool->task_count - 1; i++)
            {
                pool->tasks[i] = pool->tasks[i + 1];
            }
            pool->task_count--;

            LeaveCriticalSection(&pool->queue_lock);

            // Execute task (outside lock)
            printf("[Worker] Executing task %d\n", task.task_id);
            task.function(task.arg);
            pool->total_tasks_processed++;
        }
        else
        {
            LeaveCriticalSection(&pool->queue_lock);
        }
    }

    return 0;
}

// ============================================================================
// CREATE - Allocate thread pool with worker threads
// ============================================================================
ThreadPool *threadpool_create(int num_threads)
{
    if (num_threads < 1 || num_threads > 32)
    {
        printf("Thread count must be 1-32\n");
        return NULL;
    }

    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (pool == NULL)
    {
        printf("Failed to allocate thread pool\n");
        return NULL;
    }

    // Allocate task queue
    pool->tasks = malloc(sizeof(Task) * 1000);  // Max 1000 queued tasks
    if (pool->tasks == NULL)
    {
        printf("Failed to allocate task queue\n");
        free(pool);
        return NULL;
    }

    pool->task_count = 0;
    pool->task_capacity = 1000;
    pool->thread_count = num_threads;
    pool->total_tasks_processed = 0;

    // Allocate thread array
    pool->threads = malloc(sizeof(HANDLE) * num_threads);
    if (pool->threads == NULL)
    {
        printf("Failed to allocate thread array\n");
        free(pool->tasks);
        free(pool);
        return NULL;
    }

    // Initialize synchronization objects
    InitializeCriticalSection(&pool->queue_lock);

    pool->task_available = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pool->task_available == NULL)
    {
        printf("Failed to create task available event\n");
        free(pool->threads);
        free(pool->tasks);
        free(pool);
        return NULL;
    }

    pool->shutdown_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pool->shutdown_event == NULL)
    {
        printf("Failed to create shutdown event\n");
        CloseHandle(pool->task_available);
        free(pool->threads);
        free(pool->tasks);
        free(pool);
        return NULL;
    }

    // Create worker threads
    for (int i = 0; i < num_threads; i++)
    {
        pool->threads[i] = CreateThread(NULL, 0, worker_thread, pool, 0, NULL);
        if (pool->threads[i] == NULL)
        {
            printf("Failed to create worker thread %d\n", i);
            threadpool_free(pool);
            return NULL;
        }
        printf("[Pool] worker thread %d created\n", i);
    }

    printf("Thread pool created with %d worker threads\n", num_threads);
    return pool;
}

// ============================================================================
// FREE - Shut down pool and wait for threads
// ============================================================================
void threadpool_free(ThreadPool *pool)
{
    if (pool == NULL)
        return;

    printf("\n[Pool] Shutting down thread pool...\n");

    // Signal shutdown
    SetEvent(pool->shutdown_event);

    // Wait for all threads to complete
    WaitForMultipleObjects(pool->thread_count, pool->threads, TRUE, INFINITE);

    // Close thread handles
    for (int i = 0; i < pool->thread_count; i++)
    {
        CloseHandle(pool->threads[i]);
    }

    // Clean up
    CloseHandle(pool->task_available);
    CloseHandle(pool->shutdown_event);
    DeleteCriticalSection(&pool->queue_lock);

    free(pool->threads);
    free(pool->tasks);
    free(pool);

    printf("[Pool] Thread pool shut down\n");
}

// ============================================================================
// SUBMIT - Add task to queue (thread-safe)
// ============================================================================
//
// ALGORITHM:
// 1. Lock queue
// 2. Add task to end
// 3. Signal task_available event
// 4. Unlock queue
// ============================================================================
int threadpool_submit(ThreadPool *pool, TaskFunction func, void *arg)
{
    if (pool == NULL || func == NULL)
        return -1;

    EnterCriticalSection(&pool->queue_lock);

    // Check queue not full
    if (pool->task_count >= pool->task_capacity)
    {
        printf("Task queue full!\n");
        LeaveCriticalSection(&pool->queue_lock);
        return -1;
    }

    // Add task
    int task_id = pool->task_count;
    pool->tasks[pool->task_count].function = func;
    pool->tasks[pool->task_count].arg = arg;
    pool->tasks[pool->task_count].task_id = task_id;
    pool->task_count++;

    printf("[Pool] Task %d submitted (queue size: %d)\n", task_id, pool->task_count);

    // Signal worker that task is available
    SetEvent(pool->task_available);

    LeaveCriticalSection(&pool->queue_lock);

    return task_id;
}

// ============================================================================
// STATISTICS - Show pool usage
// ============================================================================
void threadpool_display_stats(ThreadPool *pool)
{
    if (pool == NULL)
        return;

    EnterCriticalSection(&pool->queue_lock);

    printf("\n=== Thread Pool Statistics ===\n");
    printf("Worker threads:     %d\n", pool->thread_count);
    printf("Tasks queued:       %d\n", pool->task_count);
    printf("Tasks processed:    %d\n", pool->total_tasks_processed);
    printf("Total throughput:   %d tasks\n", pool->task_count + pool->total_tasks_processed);
    printf("===============================\n\n");
    
    LeaveCriticalSection(&pool->queue_lock);
}