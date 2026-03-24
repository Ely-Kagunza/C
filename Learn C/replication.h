#ifndef REPLICATION_H
#define REPLICATION_H

#include "database.h"

// ============================================================================
// DATABASE REPLICATION
// ============================================================================
//
// PROBLEM: Single database = single point of failure
// - Hardware crash = data loss
// - No redundancy
//
// SOLUTION: Database Replication
// - Maintain primary + replica copy
// - Log all changes
// - Replica stays synchronized
// - Failover if primary fails
//
// TYPES:
// - Master-to-Slave: primary writes, replica reads only
// - Master-to-Master: both can write (more complex)
//
// Real world: PostgreSQL replication, MySQL master-slave, MongoDB replica sets
// ============================================================================

typedef enum
{
    REPLICATION_LOG_INSERT,
    REPLICATION_LOG_UPDATE,
    REPLICATION_LOG_DELETE,
} ReplicationLogType;

typedef struct
{
    ReplicationLogType type;         // What operation
    Person record;                   // Record involved
    long timestamp;                  // When it happened
} ReplicationLogEntry;

typedef struct
{
    Database *primary;               // Main database
    Database *replica;               // Backup copy

    ReplicationLogEntry *log;        // Transaction log
    int log_count;                   // How many entries
    int log_capacity;                // Allocated size

    int replicated_position;         // Last synced position (prevents duplicates)
    int is_primary_alive;            // 1 if primary OK, 0 if failed
    long last_sync_time;             // last replication time
} ReplicationManager;

// Create replication setup
ReplicationManager *replication_create(Database *primary);

// Free replication
void replication_free(ReplicationManager *rm);

// Replication insert operation
void replication_detect_changes(ReplicationManager *rm);

// Sync replica with primary (catch up on all changes)
void replication_sync(ReplicationManager *rm);

// Failover: use replica as primary
void replication_failover(ReplicationManager *rm);

// Show replication status
void replication_display_stats(ReplicationManager *rm);

#endif
