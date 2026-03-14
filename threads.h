#ifndef THREADS_H
#define THREADS_H

#include "database.h"
#include <windows.h>

// ============================================================================
// THREADING - Concurrent database access
// ============================================================================
//
// PROBLEM: Multiple threads accessing database simultaneously = race conditions
// SOLUTION: Mutex lock protects shared data
//
// EXAMPLE without mutex (BAD):
// Thread 1 reads db->count = 5
// Thread 2 also reads db->count = 5
// Thread 1 writes records[5] = alice
// Thread 2 writes records[5] = bob (overwrites alice!)
//
// EXAMPLE with mutex (GOOD):
// Thread 1 locks mutex, reads/writes, unlocks
// Thread 2 waits for mutex, then reads/writes
// No overwrites!
//
// Python equivalent: threading.Lock() or threading.RLock()
// ============================================================================

typedef struct
{
    Database *db;                  // Shared database
    CRITICAL_SECTION lock;         // Protects database access
} ThreadSafeDatabase;

typedef struct
{
    int thread_id;                 // Which thread am I?
    ThreadSafeDatabase *tsdb;      // Shared database
    int search_id;                 // What ID to search for
    Person *result;                // Where to store result
} ThreadSearchTask;

// Create thread-safe database wrapper
ThreadSafeDatabase *threadsafe_db_create(Database *db);

// Free thread-safe database
void threadsafe_db_free(ThreadSafeDatabase *tsdb);

// Thread-safe lookup (locks mutex)
Person *threadsafe_get_by_id(ThreadSafeDatabase *tsdb, int id);

// Thread-safe display (locks mutex)
void threadsafe_display(ThreadSafeDatabase *tsdb);

// Worker thread function (searches for ID)
DWORD WINAPI worker_search_thread(LPVOID arg);

#endif // THREADS_H