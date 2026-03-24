#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "replication.h"

// ============================================================================
// CREATE - Set up replication manager
// ============================================================================
ReplicationManager *replication_create(Database *primary)
{
    if (primary == NULL)
    {
        printf("Primary database is NULL\n");
        return NULL;
    }

    ReplicationManager *rm = malloc(sizeof(ReplicationManager));
    if (rm == NULL)
    {
        printf("Failed to allocate memory for replication manager\n");
        return NULL;
    }

    // Create replica (copy of primary)
    Database *replica = db_create(primary->capacity);
    if (replica == NULL)
    {
        printf("Failed to create replica database\n");
        free(rm);
        return NULL;
    }

    // Copy all records from primary to replica
    for (int i = 0; i < primary->count; i++)
    {
        db_add_record(replica, primary->records[i]);
    }

    // Replication log tracks changes
    rm->log = malloc(sizeof(ReplicationLogEntry) * 10000);  // Max 10k operations
    if (rm->log == NULL)
    {
        printf("Failed to allocate memory for replication log\n");
        db_free(replica);
        free(rm);
        return NULL;
    }

    rm->primary = primary;
    rm->replica = replica;
    rm->log_count = 0;
    rm->log_capacity = 10000;
    rm->replicated_position = primary->count;  // Track what we've seen
    rm->is_primary_alive = 1;
    rm->last_sync_time = time(NULL);

    printf("Replication manager created (moitoring primary)\n");
    printf("  Primary: %d records\n", primary->count);
    printf("  Replica: %d records (synced from primary)\n", replica->count);
    printf("\n");

    return rm;
}

// ============================================================================
// FREE - Deallocate replication manager
// ============================================================================
void replication_free(ReplicationManager *rm)
{
    if (rm == NULL)
        return;

    // Don't free primary (caller manages it)
    // But free replica and log
    db_free(rm->replica);
    free(rm->log);
    free(rm);
}

// ============================================================================
// DETECT CHANGES - Monitor primary and log new records
// ============================================================================
//
// ALGORITHM (realistic):
// 1. Compare primary record count to replica
// 2. If primary has more records, those are new
// 3. Add those new records to the replication log
// 4. Replica will read log and catch up
// ============================================================================
void replication_detect_changes(ReplicationManager *rm)
{
    if (rm == NULL)
        return;

    // Check if primary has new records since last check
    if (rm->primary->count > rm->replicated_position)
    {
        int new_records = rm->primary->count - rm->replicated_position;

        printf("[Replication] Detected %d new record(s) in primary\n", new_records);

        // Log the new records (read from primary)
        for (int i = rm->replicated_position; i < rm->primary->count; i++)
        {
            ReplicationLogEntry *entry = &rm->log[rm->log_count];
            entry->type = REPLICATION_LOG_INSERT;
            entry->record = rm->primary->records[i];  // Read from primary
            entry->timestamp = time(NULL);

            rm->log_count++;

            printf(" [Log] Logged: %s (ID %d) from primary\n", entry->record.name, entry->record.id);
        }

        // UPDATE: Remember that we've logged up to here
        rm->replicated_position = rm->primary->count;
    }
}


// ============================================================================
// SYNC - Replay log to bring replica up to date
// ============================================================================
//
// ALGORITHM: (realistic):
// 1. Read replication log entries
// 2. Apply each entry to replica
// 3. Replica now matches primary
// 4. Clear the log
// ============================================================================
void replication_sync(ReplicationManager *rm)
{
    if (rm == NULL)
    {
        printf("Replication sync failed: manager is NULL\n");
        return;
    }

    if (rm->log_count == 0)
    {
        printf("[Replication] Already in sync (0 pending changes)\n");
        return;
    }

    printf("\n[Replication] Replaying %d log entries to replica...\n", rm->log_count);

    // Apply each log entry to replica
    for (int i = 0; i < rm->log_count; i++)
    {
        ReplicationLogEntry *entry = &rm->log[i];

        if (entry->type == REPLICATION_LOG_INSERT)
        {
            db_add_record(rm->replica, entry->record);
            printf(" [Sync] Applied INSERT: %s (ID %d)\n", entry->record.name, entry->record.id);
        }
    }

    printf("[Replication] Replay complete: replica synchronized\n");
    printf("  Primary: %d records\n", rm->primary->count);
    printf("  Replica: %d records\n", rm->replica->count);

    // Clear log and update timestamp
    rm->log_count = 0;
    rm->last_sync_time = time(NULL);
    printf("[Replication] Log cleared, primary and replica in sync\n\n");
}

// ============================================================================
// FAILOVER - Switch to replica if primary fails
// ============================================================================
//
// ALGORITHM:
// 1. Check if primary is accessible
// 2. If failed, promote replica to primary role
// 3. Create new replica
// 4. Update manager pointers
// ============================================================================
void replication_failover(ReplicationManager *rm)
{
    if (rm == NULL)
        return;

    printf("\n[Replication] FAILOVER INITIATED\n");
    printf("Primary database failed, promoting replica...\n");

    // Mark primary as failed
    rm->is_primary_alive = 0;

    // Swap primary and replica
    Database *temp = rm->primary;
    rm->primary = rm->replica;
    rm->replica = temp;

    printf("[Replication] Promotion complete:\n");
    printf("  New primary: %d records\n", rm->primary->count);
    printf("  New replica: %d records\n", rm->replica->count);
    printf("[Replication] System operational on replica!\n\n");
}

// ============================================================================
// DISPLAY STATS - Show replication status
// ============================================================================
void replication_display_stats(ReplicationManager *rm)
{
    if (rm == NULL)
        return;

    time_t now = time(NULL);
    long seconds_since_sync = now - rm->last_sync_time;

    printf("\n=== Replication Status ===\n");
    printf("Primary status:     %s\n", rm->is_primary_alive ? "ALIVE" : "FAILED");
    printf("Primary records:    %d\n", rm->primary->count);
    printf("Replica records:    %d\n", rm->replica->count);
    printf("Sync status:        %s (%ld sec ago)\n",
           rm->log_count == 0 ? "IN SYNC" : "OUT OF SYNC",
           seconds_since_sync);
    printf("Pending changes:    %d\n", rm->log_count);
    printf("==========================\n\n");

    // Warning if out of sync

    if (rm->log_count > 0)
    {
        printf("⚠️  Replica behind by %d changes - run 'replication-sync'\n\n", rm->log_count);
    }
}