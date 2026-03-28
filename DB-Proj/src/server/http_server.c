#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "../../include/api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#pragma comment(lib, "ws2_32.lib")

// ================== LOGGING WITH TIMESTAMP ==================

void http_server_log(AsyncServer *server, const char *level, const char *message, ...)
{
    if (!server || !server->config.enable_logging)
        return;

    va_list args;
    va_start(args, message);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("[%s] [%s] ", timestamp, level);
    vprintf(message, args);
    printf("\n");

    va_end(args);
}

// ================== SET SOCKET NON-BLOCKING ==================

int set_nonblocking(SOCKET socket_fd)
{
    u_long mode = 1; // 1 = non-blocking, 0 = blocking

    if (ioctlsocket(socket_fd, FIONBIO, &mode) == SOCKET_ERROR)
    {
        printf(" [ERROR] ioctlsocket(FIONBIO) failed: %d\n", WSAGetLastError());
        return -1;
    }

    return 0;
}

// ================== REQUEST QUEUE ==================

RequestQueue *request_queue_create(int capacity)
{
    RequestQueue *q = malloc(sizeof(RequestQueue));
    if (!q)
        return NULL;

    q->requests = malloc((size_t)capacity * sizeof(ApiRequest));
    if (!q->requests)
    {
        free(q);
        return NULL;
    }

    q->head = 0;
    q->tail = 0;
    q->size = 0;
    q->capacity = capacity;
    InitializeCriticalSection(&q->lock);

    return q;
}

void request_queue_free(RequestQueue *q)
{
    if (!q)
        return;
    DeleteCriticalSection(&q->lock);
    free(q->requests);
    free(q);
}

int request_queue_push(RequestQueue *q, const ApiRequest *req)
{
    if (!q || !req)
        return 0;

    EnterCriticalSection(&q->lock);

    if (q->size >= q->capacity)
    {
        LeaveCriticalSection(&q->lock);
        return 0; // Queue is full
    }

    memcpy(&q->requests[q->tail], req, sizeof(ApiRequest));
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;

    LeaveCriticalSection(&q->lock);
    return 1;
}

int request_queue_pop(RequestQueue *q, ApiRequest *req)
{
    if (!q || !req)
        return 0;

    EnterCriticalSection(&q->lock);

    if (q->size == 0)
    {
        LeaveCriticalSection(&q->lock);
        return 0; // Queue is empty
    }

    memcpy(req, &q->requests[q->head], sizeof(ApiRequest));
    q->head = (q->head + 1) % q->capacity;
    q->size--;

    LeaveCriticalSection(&q->lock);
    return 1;
}

int request_queue_size(RequestQueue *q)
{
    if (!q)
        return 0;

    EnterCriticalSection(&q->lock);
    int size = q->size;
    LeaveCriticalSection(&q->lock);

    return size;
}

// ================== HTTP PARSING ==================

int http_parse_request(ClientConnection *client)
{
    if (!client)
        return 0;

    // Try to read more data (non-blocking)
    int n = recv(client->socket_fd,
                 client->request_buffer + client->bytes_received,
                 MAX_REQUEST_SIZE - client->bytes_received - 1, 0);

    if (n == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            return 0; // No data available, try again next loop
        }
        printf("[ERROR] recv() failed: %d\n", err);
        return -1; // Error
    }

    if (n == 0)
    {
        return -1; // Connection closed
    }

    client->bytes_received += n;
    client->request_buffer[client->bytes_received] = '\0';

    // Parse HTTP request line: "GET /api/people?id=1 HTTP/1.1\r\n"
    char *line_end = strstr(client->request_buffer, "\r\n");
    if (!line_end)
    {
        return 0; // Request line not complete
    }

    char path_with_query[768];
    if (sscanf(client->request_buffer, "%15s %767s", client->method, path_with_query) != 2)
    {
        return -1;
    }

    // Split path and query string
    char *query_pos = strchr(path_with_query, '?');
    if (query_pos)
    {
        *query_pos = '\0';
        strncpy(client->path, path_with_query, sizeof(client->path) - 1);
        strncpy(client->query, query_pos + 1, sizeof(client->query) - 1);
    }
    else
    {
        strncpy(client->path, path_with_query, sizeof(client->path) - 1);
        client->query[0] = '\0';
    }

    // Look for end of headers (blank line: \r\n\r\n)
    char *headers_end = strstr(client->request_buffer, "\r\n\r\n");
    if (!headers_end)
    {
        return 0; // Headers not complete
    }

    // Extract Content-Length
    char *cl_header = strstr(client->request_buffer, "Content-Length:");
    if (cl_header)
    {
        sscanf(cl_header, "Content-Length: %d", &client->content_length);
    }
    else
    {
        client->content_length = 0;
    }

    // Check if we have the full body
    char *body_start = headers_end + 4;
    size_t body_received = client->bytes_received - (body_start - client->request_buffer);

    if (body_received < (size_t)client->content_length)
    {
        return 0; // Body not complete yet
    }

    // Full request received!
    if (client->content_length > 0)
    {
        client->body = malloc((size_t)client->content_length + 1);
        if (client->body)
        {
            memcpy(client->body, body_start, (size_t)client->content_length);
            client->body[client->content_length] = '\0';
        }
    }
    else
    {
        client->body = malloc(1);
        if (client->body)
            client->body[0] = '\0';
    }

    client->request_complete = 1;
    return 1; // Request ready to process
}

void http_build_response(ClientConnection *client, const char *body, int status_code)
{
    if (!client || !body)
        return;

    const char *status_text = "OK";
    switch (status_code)
    {
    case 200:
        status_text = "OK";
        break;
    case 201:
        status_text = "Created";
        break;
    case 400:
        status_text = "Bad Request";
        break;
    case 404:
        status_text = "Not Found";
        break;
    case 500:
        status_text = "Internal Server Error";
        break;
    default:
        status_text = "Unknown";
        break;
    }

    size_t body_len = strlen(body);
    client->response_length = snprintf(client->response_buffer,
                                       sizeof(client->response_buffer) - 1,
                                       "HTTP/1.1 %d %s\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Content-Length: %zu\r\n"
                                       "Connection: close\r\n"
                                       "\r\n"
                                       "%s",
                                       status_code, status_text, body_len, body);

    client->response_sent = 0;
}

// ================== SERVICE LIFECYCLE ==================

AsyncServer *http_server_create(ServerConfig config, Database *db, BTreeIndex *age_idx, BTreeIndex *salary_idx, HashIndex *id_idx)
{
    AsyncServer *server = malloc(sizeof(AsyncServer));
    if (!server)
        return NULL;

    memset(server, 0, sizeof(AsyncServer));

    server->listen_socket = INVALID_SOCKET;
    server->config = config;
    server->db = db;
    server->age_index = age_idx;
    server->salary_index = salary_idx;
    server->id_index = id_idx;
    server->max_clients = config.max_clients;

    server->clients = malloc((size_t)config.max_clients * sizeof(ClientConnection));
    if (!server->clients)
    {
        free(server);
        return NULL;
    }

    memset(server->clients, 0, (size_t)config.max_clients * sizeof(ClientConnection));

    // Initialize all client sockets to INVALID_SOCKET
    for (int i = 0; i < config.max_clients; i++)
    {
        server->clients[i].socket_fd = INVALID_SOCKET;
    }

    InitializeCriticalSection(&server->clients_lock);
    InitializeCriticalSection(&server->db_lock);

    server->queue = request_queue_create(config.queue_size);
    if (!server->queue)
    {
        free(server->clients);
        free(server);
        return NULL;
    }

    http_server_log(server, "INFO", "AsyncServer created (max_clients=%d, port=%d)",
                    config.max_clients, config.port);

    return server;
}

int http_server_start(AsyncServer *server)
{
    if (!server)
        return 0;

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        if (server->config.enable_logging)
        {
            printf("[ERROR] WSAStartup failed\n");
        }
        return 0;
    }

    server->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->listen_socket == INVALID_SOCKET)
    {
        if (server->config.enable_logging)
        {
            printf("[ERROR] socket() failed: %d\n", WSAGetLastError());
        }
        WSACleanup();
        return 0;
    }

    // Allow socket reuse
    int reuse = 1;
    setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    // Set non-blocking
    if (set_nonblocking(server->listen_socket) == -1)
    {
        closesocket(server->listen_socket);
        WSACleanup();
        return 0;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(server->config.port);

    if (bind(server->listen_socket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        if (server->config.enable_logging)
        {
            printf("[ERROR] bind() failed: %d\n", WSAGetLastError());
        }
        closesocket(server->listen_socket);
        WSACleanup();
        return 0;
    }

    if (listen(server->listen_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        if (server->config.enable_logging)
        {
            printf("[ERROR] listen() failed: %d\n", WSAGetLastError());
        }
        closesocket(server->listen_socket);
        WSACleanup();
        return 0;
    }

    server->running = 1;
    http_server_log(server, "INFO", "Server listening on http://127.0.0.1:%d",
                    server->config.port);

    return 1;
}

void http_server_stop(AsyncServer *server)
{
    if (!server)
        return;
    server->running = 0;
}

void http_server_free(AsyncServer *server)
{
    if (!server)
        return;

    http_server_stop(server);

    if (server->listen_socket != INVALID_SOCKET)
    {
        closesocket(server->listen_socket);
    }

    for (int i = 0; i < server->max_clients; i++)
    {
        if (server->clients[i].socket_fd != INVALID_SOCKET)
        {
            http_close_client(server, &server->clients[i]);
        }
    }

    DeleteCriticalSection(&server->clients_lock);
    DeleteCriticalSection(&server->db_lock);

    request_queue_free(server->queue);
    free(server->clients);

    WSACleanup();
    free(server);
}

// ================== CLIENT MANAGEMENT ==================

ClientConnection *http_accept_client(AsyncServer *server)
{
    if (!server || server->num_clients >= server->max_clients)
    {
        return NULL;
    }

    struct sockaddr_in client_addr = {0};
    int client_addr_len = sizeof(client_addr);

    SOCKET client_socket = accept(server->listen_socket,
                                  (struct sockaddr *)&client_addr,
                                  &client_addr_len);

    if (client_socket == INVALID_SOCKET)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            http_server_log(server, "ERROR", "accept() failed: %d", err);
        }
        return NULL;
    }

    // Set non-blocking
    if (set_nonblocking(client_socket) == -1)
    {
        closesocket(client_socket);
        return NULL;
    }

    // Find empty slot
    ClientConnection *client = NULL;
    for (int i = 0; i < server->max_clients; i++)
    {
        if (server->clients[i].socket_fd == INVALID_SOCKET)
        {
            client = &server->clients[i];
            break;
        }
    }

    if (!client)
    {
        closesocket(client_socket);
        return NULL;
    }

    memset(client, 0, sizeof(ClientConnection));
    client->socket_fd = client_socket;
    client->state = CLIENT_READING;
    client->connected_time = time(NULL);

    if (inet_ntop(AF_INET, &client_addr.sin_addr, client->client_addr,
                  sizeof(client->client_addr)) == NULL)
    {
        client->client_addr[0] = '\0';
    }
    client->client_port = ntohs(client_addr.sin_port);

    snprintf(client->request_id, sizeof(client->request_id) - 1,
             "%s:%d-%llu", client->client_addr, client->client_port, server->total_requests);

    server->num_clients++;
    if (server->num_clients > server->peak_concurrent_clients)
    {
        server->peak_concurrent_clients = server->num_clients;
    }

    http_server_log(server, "INFO", "Client %s accepted (ID: %s)",
                    client->client_addr, client->request_id);

    return client;
}

void http_handle_client_read(AsyncServer *server, ClientConnection *client)
{
    if (!server || !client)
        return;

    int result = http_parse_request(client);

    if (result == -1)
    {
        client->state = CLIENT_CLOSING;
        return;
    }

    if (result == 1)
    {
        // Full request received, dispatch to API
        server->total_requests++;
        http_server_log(server, "INFO", "[%s] %s %s (%d bytes body)",
                        client->request_id, client->method, client->path,
                        client->content_length);

        client->state = CLIENT_PROCESSING;
    }
}

void http_handle_client_write(AsyncServer *server, ClientConnection *client)
{
    if (!server || !client)
        return;

    if (client->state != CLIENT_WRITING)
        return;

    size_t remaining = client->response_length - client->response_sent;
    if (remaining == 0)
    {
        client->state = CLIENT_CLOSING;
        return;
    }

    int n = send(client->socket_fd,
                 client->response_buffer + client->response_sent,
                 (int)remaining, 0);

    if (n == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            http_server_log(server, "ERROR", "send() failed: %d", err);
            client->state = CLIENT_CLOSING;
        }
        return;
    }

    client->response_sent += n;

    if (client->response_sent >= client->response_length)
    {
        http_server_log(server, "INFO", "[%s] Response sent (%zu bytes)",
                        client->request_id, client->response_length);
        client->state = CLIENT_CLOSING;
    }
}

void http_close_client(AsyncServer *server, ClientConnection *client)
{
    if (!server || !client)
        return;

    time_t now = time(NULL);
    client->latency_ms = (int)((now - client->connected_time) * 1000);

    http_server_log(server, "INFO", "Closing client %s (latency: %dms)",
                    client->request_id, client->latency_ms);

    if (client->socket_fd != INVALID_SOCKET)
    {
        closesocket(client->socket_fd);
        client->socket_fd = INVALID_SOCKET;
    }

    if (client->body)
    {
        free(client->body);
        client->body = NULL;
    }

    memset(client, 0, sizeof(ClientConnection));
    client->socket_fd = INVALID_SOCKET;

    server->num_clients--;
}

// ================== EVENT LOOP (THE HEART) ==================

int http_server_run(AsyncServer *server)
{
    if (!server || server->listen_socket == INVALID_SOCKET)
    {
        return -1;
    }

    http_server_log(server, "INFO", "Event loop starting...");

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (server->running)
    {
        // Build fd_set for select()
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        // Always monitor listening socket
        FD_SET(server->listen_socket, &read_fds);

        // Monitor each client based on state
        for (int i = 0; i < server->max_clients; i++)
        {
            ClientConnection *client = &server->clients[i];

            if (client->socket_fd == INVALID_SOCKET)
                continue;

            // Readable: waiting for request
            if (client->state == CLIENT_READING)
            {
                FD_SET(client->socket_fd, &read_fds);
            }

            // Writable: sending response
            if (client->state == CLIENT_WRITING)
            {
                FD_SET(client->socket_fd, &write_fds);
            }
        }

        // select() with timeout
        int activity = select(0, &read_fds, &write_fds, NULL, &timeout);

        if (activity == SOCKET_ERROR)
        {
            http_server_log(server, "ERROR", "select() failed: %d", WSAGetLastError());
            break;
        }

        // Accept new connections
        if (FD_ISSET(server->listen_socket, &read_fds))
        {
            ClientConnection *new_client = http_accept_client(server);
            if (!new_client && server->num_clients >= server->max_clients)
            {
                http_server_log(server, "WARN", "Max clients (%d) reached", server->max_clients);
            }
        }

        // Handle existing clients
        for (int i = 0; i < server->max_clients; i++)
        {
            ClientConnection *client = &server->clients[i];

            if (client->socket_fd == INVALID_SOCKET)
                continue;

            // Check timeout
            time_t now = time(NULL);
            if ((now - client->connected_time) > (time_t)server->config.request_timeout_seconds)
            {
                http_server_log(server, "WARN", "Client %s timeout", client->request_id);
                client->state = CLIENT_CLOSING;
            }

            // Readable
            if (FD_ISSET(client->socket_fd, &read_fds))
            {
                http_handle_client_read(server, client);
            }

            // Writable
            if (FD_ISSET(client->socket_fd, &write_fds))
            {
                http_handle_client_write(server, client);
            }

            // Processing → Dispatch to API
            if (client->state == CLIENT_PROCESSING && client->request_complete)
            {
                api_dispatch(server, client);
                client->state = CLIENT_WRITING;
                client->request_complete = 0;
            }

            // Close finished clients
            if (client->state == CLIENT_CLOSING)
            {
                http_close_client(server, client);
            }
        }
    }

    http_server_log(server, "INFO", "Event loop stopped");
    return 0;
}

// ================= STATS ==================

void http_server_display_stats(AsyncServer *server)
{
    if (!server)
        return;

    printf("\n═══════════════════════════════════════════════════════\n");
    printf("           HTTP Server Statistics\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    printf("Total Requests:           %llu\n", server->total_requests);
    printf("Total Errors:             %llu\n", server->total_errors);
    printf("Active Clients:           %d/%d\n", server->num_clients, server->max_clients);
    printf("Peak Concurrent:          %d\n", server->peak_concurrent_clients);
    printf("Request Queue Size:       %d\n", request_queue_size(server->queue));
    printf("═══════════════════════════════════════════════════════\n\n");
}