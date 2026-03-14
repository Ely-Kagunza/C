#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "threads.h"
#include "database.h"

// ============================================================================
// CREATE - Thread-safe database wrapper
// ============================================================================
ThreadSafeDatabase *threadsafe_db_create(Database *db)
{
    if (db == NULL)
    {
        printf("Database is NULL\n");
        return NULL;
    }

    ThreadSafeDatabase *tsdb = malloc(sizeof(ThreadSafeDatabase));
    if (tsdb == NULL)
    {
        printf("Failed to allocate thread-safe database\n");
        return NULL;
    }

    tsdb->db = db;

    // Initialize critical section (Windows equivalent of mutex)
    InitializeCriticalSection(&tsdb->lock);

    printf("Thread-safe database wrapper created\n");
    return tsdb;
}

// ============================================================================
// FREE - Deallocate thread-safe database
// ============================================================================
void threadsafe_db_free(ThreadSafeDatabase *tsdb)
{
    if (tsdb == NULL)
        return;

    DeleteCriticalSection(&tsdb->lock);
    free(tsdb);
    printf("Thread-safe database wrapper freed\n");
}

// ============================================================================
// LOOKUP - Thread-safe get by ID with critical section lock
// ============================================================================
//
// ALGORITHM:
// 1. EnterCriticalSection (wait if another thread has it)
// 2. Perform lookup
// 3. LeaveCriticalSection (allow other threads to proceed)
// ============================================================================
Person *threadsafe_get_by_id(ThreadSafeDatabase *tsdb, int id)
{
    if (tsdb == NULL)
        return NULL;

    // LOCK: Acquire critical section
    EnterCriticalSection(&tsdb->lock);

    // Critical section (protected)
    Person *result = db_get_by_id(tsdb->db, id);

    // UNLOCK: Release critical section
    LeaveCriticalSection(&tsdb->lock);

    return result;
}

// ============================================================================
// DISPLAY - Thread-safe display
// ============================================================================
void threadsafe_display(ThreadSafeDatabase *tsdb)
{
    if (tsdb == NULL)
        return;

    EnterCriticalSection(&tsdb->lock);
    db_display(tsdb->db);
    LeaveCriticalSection(&tsdb->lock);
}

// ============================================================================
// WORKER THREAD - Search for ID (Windows version)
// ============================================================================
//
// Windows thread function signature:
// DWORD WINAPI function_name(LPVOID arg)
// ============================================================================
DWORD WINAPI worker_search_thread(LPVOID arg)
{
    ThreadSearchTask *task = (ThreadSearchTask *)arg;

    printf("[Thread %d] Starting search for ID %d\n", task->thread_id, task->search_id);

    // Thread-safe lookup (blocks if another thread has lock)
    Person *found = threadsafe_get_by_id(task->tsdb, task->search_id);

    if (found != NULL)
    {
        printf("[Thread %d] Found: %s (Age %d)\n", task->thread_id, found->name, found->age);
        task->result = found;
    }
    else
    {
        printf("[Thread %d] Not found\n", task->thread_id);
        task->result = NULL;
    }

    return 0;
}