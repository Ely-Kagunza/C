#ifndef ASYNC_SERVER_H
#define ASYNC_SERVER_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

// ================== CLIENT STATE MACHINE ==================

typedef enum {
    CLIENT_ACCEPTING,             // Just accepted
    CLIENT_READING,               // Reading HTTP request
    CLIENT_PROCESSING,            // Waiting for inference result
    CLIENT_WRITING,               // Writing HTTP response
    CLIENT_CLOSING                // Closing connection
} ClientState;

typedef struct {
    SOCKET socket_fd;                // Windows socket handle
    ClientState state;            // Current state

    // Request buffer (for reading HTTP)
    char request_buffer[4096];
    size_t bytes_received;
    int content_length;
    int request_complete;

    // Request parsing
    char method[16];              // "GET", "POST", etc.
    char path[256];               // "/v1/infer"
    char *body;                   // Request body

    // Response buffer (for writing)
    char response_buffer[8192];
    size_t response_sent;
    size_t response_total;

    // Timing
    time_t connected_time;
    int latency_ms;

    // Request ID for tracing
    char request_id[64];

    // Client info
    char client_addr[32];            // "127.0.0.1:54321"
    int client_port;
} ClientConnection;

// ================== INFERENCE REQUEST QUEUE ==================

typedef struct {
    char *query;                    // SQL query to execute
    char request_id[64];
    int client_socket;              // Which client to send response to
} InferenceRequest;

typedef struct {
    InferenceRequest *requests;
    int head;
    int tail;
    int size;
    int capacity;
} RequestQueue;

// ================== SERVER CONFIGURATION ==================

typedef struct {
    int port;
    int max_clients;
    int queue_size;
    int request_timeout_seconds;
    int enable_logging;
} ServerConfig;

// ================== EVENT LOOP SERVER STATE ==================

typedef struct {
    int listen_socket;
    int running;

    // Connected clients
    ClientConnection *clients;
    int num_clients;
    int max_clients;

    // Request queue (for inference worker)
    RequestQueue *queue;

    // Metrics
    uint64_t total_requests;
    uint64_t total_errors;
    int peak_concurrent_clients;

    // Config
    ServerConfig config;
} AsyncServer;

// ================== FUNCTION DECLARATIONS ==================

// Server lifecycle
AsyncServer *async_server_create(ServerConfig config);
int async_server_start(AsyncServer *server, const char *bind_addr);
int async_server_run(AsyncServer *server);
void async_server_stop(AsyncServer *server);
void async_server_free(AsyncServer *server);

// Client management
ClientConnection* async_server_accept_client(AsyncServer *server);
void async_server_handle_client_read(AsyncServer *server, ClientConnection *client);
void async_server_handle_client_write(AsyncServer *server, ClientConnection *client);
void async_server_close_client(AsyncServer *server, ClientConnection *client);

// HTTP parsing
int parse_http_request(ClientConnection *client);
void build_http_response(ClientConnection *client, const char *body, int status_code);

// Logging
void server_log(AsyncServer *server, const char *level, const char *message, ...);

// Display stats
void async_server_display_stats(AsyncServer *server);

#endif