#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "../../include/http_server.h"
#include "../../include/api.h"
#include "../../include/hash_index.h"
#include <stdio.h>
#include <signal.h>

#define DATA_FILE "people.txt"

// Global server reference for signal handler
AsyncServer *g_server = NULL;

void signal_handler(int sig)
{
    if (sig == SIGINT)
    {
        printf("\n[INFO] Shutting down server...\n");
        if (g_server)
        {
            http_server_stop(g_server);
        }
    }
}

void print_banner(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("         Database HTTP API Server (Phase 10)\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("\n");
}

void print_usage(void)
{
    printf("API Endpoints:\n");
    printf("  GET    /api/health                    - Server status\n");
    printf("  GET    /api/people                    - List all people\n");
    printf("  GET    /api/people?age_min=X&age_max=Y - Age range query\n");
    printf("  GET    /api/people?salary_min=X&salary_max=Y - Salary range\n");
    printf("  GET    /api/people/:id                - Get by ID\n");
    printf("  POST   /api/people                    - Add person\n");
    printf("  PUT    /api/people/:id                - Update person\n");
    printf("  DELETE /api/people/:id                - Delete person\n");
    printf("\n");
    printf("Test with curl:\n");
    printf("  curl http://127.0.0.1:8080/api/health\n");
    printf("  curl http://127.0.0.1:8080/api/people\n");
    printf("  curl http://127.0.0.1:8080/api/people?age_min=25&age_max=50\n");
    printf("  curl -X POST -H \"Content-Type: application/json\" \\\n");
    printf("       -d '{\"id\":100,\"name\":\"John\",\"age\":30,\"salary\":50000}' \\\n");
    printf("       http://127.0.0.1:8080/api/people\n");
    printf("\n");
    printf("Press Ctrl+C to stop the server.\n\n");
}

int main(int argc, char *argv[])
{
    print_banner();

    // Load database from file
    printf("[INFO] Loading database from %s...\n", DATA_FILE);
    Database *db = database_load_text(DATA_FILE);
    if (!db)
    {
        printf("[ERROR] Failed to load database from %s\n", DATA_FILE);
        printf("[INFO] Make sure '%s' exists in the current directory.\n", DATA_FILE);
        return 1;
    }
    printf("[INFO] Loaded %d people from database\n\n", db->count);

    // Create B-tree indexes
    printf("[INFO] Creating B-tree indexes...\n");
    BTreeIndex *age_index = btree_create(5, 0);    // 0 = age field
    BTreeIndex *salary_index = btree_create(5, 1); // 1 = salary field

    if (!age_index || !salary_index)
    {
        printf("[ERROR] Failed to create B-tree indexes\n");
        database_free(db);
        return 1;
    }

    // Build indexes from database
    printf("[INFO] Building age index...\n");
    if (!btree_build_from_database(age_index, db))
    {
        printf("[ERROR] Failed to build age index\n");
        btree_free(age_index);
        btree_free(salary_index);
        database_free(db);
        return 1;
    }

    printf("[INFO] Building salary index...\n");
    if (!btree_build_from_database(salary_index, db))
    {
        printf("[ERROR] Failed to build salary index\n");
        btree_free(age_index);
        btree_free(salary_index);
        database_free(db);
        return 1;
    }

    printf("[INFO] Age B-tree height: %d\n", btree_get_height(age_index));
    printf("[INFO] Salary B-tree height: %d\n\n", btree_get_height(salary_index));

    // Create hash index for O(1) ID lookups
    printf("[INFO] Creating hash index...");
    HashIndex *id_index = hash_index_create(1024); // 1024 buckets
    if (!id_index)
    {
        printf("[ERROR] Failed to create hash index\n");
        btree_free(age_index);
        btree_free(salary_index);
        database_free(db);
        return 1;
    }

    printf("[INFO] Building hash index...");
    if (!hash_index_build(id_index, db))
    {
        printf("[ERROR] Failed to build hash index\n");
        hash_index_free(id_index);
        btree_free(age_index);
        btree_free(salary_index);
        database_free(db);
        return 1;
    }
    printf("[INFO] Hash index built with %d records\n\n", db->count);

    // Create server configuration
    ServerConfig config = {
        .port = HTTP_PORT,
        .max_clients = MAX_CONCURRENT_CLIENTS,
        .queue_size = 128,
        .request_timeout_seconds = 30,
        .enable_logging = 1};

    // Create HTTP server
    printf("[INFO] Creating HTTP server...\n");
    AsyncServer *server = http_server_create(config, db, age_index, salary_index, id_index);
    if (!server)
    {
        printf("[ERROR] Failed to create HTTP server\n");
        hash_index_free(id_index);
        btree_free(age_index);
        btree_free(salary_index);
        database_free(db);
        return 1;
    }

    // Store global reference for signal handler
    g_server = server;
    signal(SIGINT, signal_handler);

    // Start server
    printf("[INFO] Starting HTTP server...\n");
    if (!http_server_start(server))
    {
        printf("[ERROR] Failed to start HTTP server\n");
        http_server_free(server);
        hash_index_free(id_index);
        btree_free(age_index);
        btree_free(salary_index);
        database_free(db);
        return 1;
    }

    print_usage();

    // Run event loop
    printf("[INFO] Event loop running...\n\n");
    http_server_run(server);

    // Cleanup
    printf("\n[INFO] Cleaning up...\n");
    http_server_display_stats(server);

    http_server_free(server);
    hash_index_free(id_index);
    btree_free(age_index);
    btree_free(salary_index);
    database_free(db);

    printf("[INFO] Server stopped gracefully\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    return 0;
}