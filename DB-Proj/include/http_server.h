#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#define _CRT_SECURE_NO_WARNINGS
#include "database.h"
#include "btree_index.h"
#include "hash_index.h"
#include <stdint.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define HTTP_PORT 8080
#define MAX_REQUEST_SIZE 4096
#define MAX_RESPONSE_SIZE 65536
#define MAX_CONCURRENT_CLIENTS 32

// ==================  CLIENT STATE MACHINE ==================

typedef enum
{
    CLIENT_ACCEPTING,  // Just accepted connection
    CLIENT_READING,    // Reading HTTP request
    CLIENT_PROCESSING, // Processing API request
    CLIENT_WRITING,    // Writing HTTP response
    CLIENT_CLOSING     // Closing connection
} ClientState;

typedef struct
{
    SOCKET socket_fd;  // Windows socket handle
    ClientState state; // Current state

    // Request buffers
    char request_buffer[MAX_REQUEST_SIZE];
    size_t bytes_received;
    int content_length;
    int request_complete;

    // Parsed request
    char method[16]; // "GET", "POST", etc.
    char path[256];  // "/api/people"
    char query[512]; // key=value&key2=value2
    char *body;      // POST/PUT body

    // Response buffer
    char response_buffer[MAX_RESPONSE_SIZE];
    size_t response_length;
    size_t response_sent;

    // Timing
    time_t connected_time;
    int latency_ms;

    // Client info
    char client_addr[32]; // "127.0.0.1:54321"
    int client_port;

    // Tracking
    char request_id[64];
} ClientConnection;

// ==================  API REQUEST QUEUE ==================

typedef struct
{
    char method[16];
    char path[256];
    char query[512];
    char body[2048];
    int client_socket; // Which client to send response to
    char request_id[64];
} ApiRequest;

typedef struct
{
    ApiRequest *requests;
    int head;
    int tail;
    int size;
    int capacity;
    CRITICAL_SECTION lock;
} RequestQueue;

// ==================  SERVER CONFIGURATION ==================

typedef struct
{
    int port;
    int max_clients;
    int queue_size;
    int request_timeout_seconds;
    int enable_logging;
} ServerConfig;

// ================== ASYNC SERVER STATE ==================

typedef struct
{
    SOCKET listen_socket;
    int running;

    // Connected clients
    ClientConnection *clients;
    int num_clients;
    int max_clients;
    CRITICAL_SECTION clients_lock;

    // Request queue (for worker thread)
    RequestQueue *queue;

    // Worker thread
    HANDLE worker_thread;

    // Database & indexes
    Database *db;
    BTreeIndex *age_index;
    BTreeIndex *salary_index;
    HashIndex *id_index;
    CRITICAL_SECTION db_lock;

    // Metrics
    uint64_t total_requests;
    uint64_t total_errors;
    int peak_concurrent_clients;

    // Configuration
    ServerConfig config;
} AsyncServer;

// ================== FUNCTION DECLARATIONS ==================

// Server lifecycle
AsyncServer *http_server_create(ServerConfig config, Database *db,
                                BTreeIndex *age_idx, BTreeIndex *salary_idx, HashIndex *id_idx);
int http_server_start(AsyncServer *server);
int http_server_run(AsyncServer *server);
void http_server_stop(AsyncServer *server);
void http_server_free(AsyncServer *server);

// Client management
ClientConnection *http_accept_client(AsyncServer *server);
void http_handle_client_read(AsyncServer *server, ClientConnection *client);
void http_handle_client_write(AsyncServer *server, ClientConnection *client);
void http_close_client(AsyncServer *server, ClientConnection *client);

// Request queue
RequestQueue *request_queue_create(int capacity);
void request_queue_free(RequestQueue *q);
int request_queue_push(RequestQueue *q, const ApiRequest *req);
int request_queue_pop(RequestQueue *q, ApiRequest *req);
int request_queue_size(RequestQueue *q);

// HTTP parsing
int http_parse_request(ClientConnection *client);
void http_build_response(ClientConnection *client, const char *body, int status_code);

// Worker thread
DWORD WINAPI http_worker_thread(LPVOID arg);

// Logging
void http_server_log(AsyncServer *server, const char *level, const char *message, ...);

// Stats
void http_server_display_stats(AsyncServer *server);

#endif // HTTP_SERVER_H
