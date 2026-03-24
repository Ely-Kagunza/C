#define _CRT_SECURE_NO_WARNINGS
#include "async_server_windows.h"
#include <stdarg.h>

// ============== UTILITY: LOGGING ==============

void server_log(AsyncServer *server, const char *level, const char *message, ...) {
    if (!server || !server->config.enable_logging) return;
    
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

// ============== UTILITY: SET SOCKET NON-BLOCKING ==============

// Windows: Use ioctlsocket() with FIONBIO flag
// CRITICAL: This is HOW windows dows non-blocking, not fcntl()
int set_nonblocking(SOCKET socket_fd) {
    u_long mode = 1;  // 1 = non-blocking, 0 = blocking

    if (ioctlsocket(socket_fd, FIONBIO, &mode) == SOCKET_ERROR) {
        printf("ioctlsocket(FIONBIO) failed: %d\n", WSAGetLastError());
        return -1;
    }

    return 0;
}

// ============== REQUEST QUEUE ==============

RequestQueue* queue_create(int capacity) {
    RequestQueue *q = malloc(sizeof(RequestQueue));
    q->capacity = capacity;
    q->size = 0;
    q->head = 0;
    q->tail = 0;
    q->requests = malloc(sizeof(InferenceRequest) * capacity);
    
    memset(q->requests, 0, sizeof(InferenceRequest) * capacity);
    return q;
}

void queue_free(RequestQueue *q) {
    if (!q) return;
    free(q->requests);
    free(q);
}

// ============== SERVER CREATION ==============

AsyncServer* async_server_create(ServerConfig config) {
    AsyncServer *server = malloc(sizeof(AsyncServer));
    if (!server) {
        fprintf(stderr, "malloc(AsyncServer) failed\n");
        return NULL;
    }
    
    memset(server, 0, sizeof(AsyncServer));
    
    server->listen_socket = INVALID_SOCKET;
    server->max_clients = config.max_clients;
    server->config = config;
    
    // Allocate client array
    server->clients = malloc(sizeof(ClientConnection) * config.max_clients);
    if (!server->clients) {
        fprintf(stderr, "malloc(clients) failed\n");
        free(server);
        return NULL;
    }
    
    memset(server->clients, 0, sizeof(ClientConnection) * config.max_clients);
    
    // Initialize client sockets to INVALID SOCKET (unsused)
    for (int i = 0; i < config.max_clients; i++) {
        server->clients[i].socket_fd = INVALID_SOCKET;
    }
    
    // Create request queue
    server->queue = queue_create(config.queue_size);
    if (!server->queue) {
        fprintf(stderr, "queue_create failed\n");
        free(server->clients);
        free(server);
        return NULL;
    }
    
    server_log(server, "INFO", "AsyncServer created (max_clients=%d)", config.max_clients);
    
    return server;
}

// ============== START SERVER (CREATE LISTENING SOCKET) ==============

int async_server_start(AsyncServer *server, const char *bind_addr) {
    if (!server) return -1;
    
    // Create listening socket
    server->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->listen_socket == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        return -1;
    }
    
    // Allow reusing address (important for quick restart)
    int reuse = 1;
    if (setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (const char *)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
        printf("setsockopt(SO_REUSEADDR) failed: %d\n", WSAGetLastError());
        closesocket(server->listen_socket);
        return -1;
    }
    
    // Set non-blocking
    if (set_nonblocking(server->listen_socket) == -1) {
        closesocket(server->listen_socket);
        return -1;
    }
    
    // Bind to address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->config.port);
    
    if (inet_pton(AF_INET, bind_addr, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid bind address: %s\n", bind_addr);
        closesocket(server->listen_socket);
        return -1;
    }
    
    if (bind(server->listen_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("bind() failed: %d\n", WSAGetLastError());
        closesocket(server->listen_socket);
        return -1;
    }
    
    // Listen for connections
    if (listen(server->listen_socket, 128) == SOCKET_ERROR) {
        printf("listen() failed: %d\n", WSAGetLastError());
        closesocket(server->listen_socket);
        return -1;
    }
    
    server->running = 1;
    server_log(server, "INFO", "Server listening on %s:%d", 
               bind_addr, server->config.port);
    
    return 0;
}

// ============== ACCEPT NEW CLIENT ==============

ClientConnection* async_server_accept_client(AsyncServer *server) {
    if (!server || server->num_clients >= server->max_clients) {
        return NULL;
    }
    
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    
    SOCKET client_socket = accept(server->listen_socket, 
                               (struct sockaddr *)&client_addr,
                               &client_addr_len);
    
    if (client_socket == INVALID_SOCKET) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            printf("accept() failed: %d\n", err);
        }
        return NULL;
    }
    
    // Set non-blocking
    if (set_nonblocking(client_socket) == -1) {
        closesocket(client_socket);
        return NULL;
    }
    
    // Find empty slot in client array
    ClientConnection *client = NULL;
    for (int i = 0; i < server->max_clients; i++) {
        if (server->clients[i].socket_fd == INVALID_SOCKET) {
            client = &server->clients[i];
            break;
        }
    }
    
    if (!client) {
        closesocket(client_socket);
        return NULL;
    }
    
    // Initialize client
    memset(client, 0, sizeof(ClientConnection));
    client->socket_fd = client_socket;
    client->state = CLIENT_READING;
    client->connected_time = time(NULL);
    
    // Store client address
    inet_ntop(AF_INET, &client_addr.sin_addr, client->client_addr, 
              sizeof(client->client_addr));
    client->client_port = ntohs(client_addr.sin_port);
    
    // Generate request ID
    snprintf(client->request_id, sizeof(client->request_id), 
             "%lld-%d", (long long)time(NULL), (int)client_socket);
    
    server->num_clients++;
    if (server->num_clients > server->peak_concurrent_clients) {
        server->peak_concurrent_clients = server->num_clients;
    }
    
    server_log(server, "INFO", "Client %s:%d connected (ID: %s)", 
               client->client_addr, client->client_port, client->request_id);
    
    return client;
}

// ============== CLOSE CLIENT CONNECTION ==============

void async_server_close_client(AsyncServer *server, ClientConnection *client) {
    if (!client || client->socket_fd == INVALID_SOCKET) return;
    
    // Record latency
    time_t now = time(NULL);
    client->latency_ms = (int)((now - client->connected_time) * 1000);
    
    server_log(server, "INFO", "Closing client %s:%d (latency: %dms)", 
               client->client_addr, client->client_port, client->latency_ms);
    
    closesocket(client->socket_fd);
    
    if (client->body) {
        free(client->body);
    }
    
    memset(client, 0, sizeof(ClientConnection));
    client->socket_fd = INVALID_SOCKET;  // Mark as unused
    
    server->num_clients--;
}

// ============== PARSE HTTP REQUEST ==============

int parse_http_request(ClientConnection *client) {
    // Try to read more data
    int n = recv(client->socket_fd, 
                     client->request_buffer + client->bytes_received,
                     (int)(sizeof(client->request_buffer) - client->bytes_received - 1), 0);
    
    if (n == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            // No data available yet, try again next loop
            return 0;  // Not ready
        }
        printf("recv() failed: %d\n", err);
        return -1;  // Error
    }
    
    if (n == 0) {
        // Client closed connection
        return -1;
    }
    
    client->bytes_received += n;
    client->request_buffer[client->bytes_received] = '\0';
    
    // Parse HTTP request line
    // Format: "POST /v1/infer HTTP/1.1\r\n"
    char *line_end = strstr(client->request_buffer, "\r\n");
    if (!line_end) {
        // Request line not complete yet
        return 0;
    }
    
    sscanf(client->request_buffer, "%15s %255s", 
           client->method, client->path);
    
    // Look for end of headers (blank line: \r\n\r\n)
    char *headers_end = strstr(client->request_buffer, "\r\n\r\n");
    if (!headers_end) {
        // Headers not complete, need more data
        return 0;
    }
    
    // Extract Content-Length
    char *cl_header = strstr(client->request_buffer, "Content-Length:");
    if (cl_header) {
        sscanf(cl_header, "Content-Length: %d", &client->content_length);
    } else {
        client->content_length = 0;
    }
    
    // Check if we have the full body
    char *body_start = headers_end + 4;
    size_t body_received = client->bytes_received - (body_start - client->request_buffer);
    
    if (body_received < client->content_length) {
        // Body not complete yet
        return 0;
    }
    
    // Full request received!
    client->body = malloc(client->content_length + 1);
    memcpy(client->body, body_start, client->content_length);
    client->body[client->content_length] = '\0';
    
    client->request_complete = 1;
    return 1;  // Request ready to process
}

// ============== BUILD HTTP RESPONSE ==============

void build_http_response(ClientConnection *client, const char *body, int status_code) {
    const char *status_text = "OK";
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        case 503: status_text = "Service Unavailable"; break;
    }
    
    int content_length = strlen(body);
    
    snprintf(client->response_buffer, sizeof(client->response_buffer),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status_code, status_text, content_length, body);
    
    client->response_total = strlen(client->response_buffer);
    client->response_sent = 0;
}

// ============== EVENT LOOP: THE HEART OF THE SERVER ==============

int async_server_run(AsyncServer *server) {
    if (!server || server->listen_socket == INVALID_SOCKET) {
        return -1;
    }

    server_log(server, "INFO", "Event loop starting...");
    
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    while (server->running) {
        // Build fd_set for select()
        // This tells select() which sockets to monitor
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        
        // Always monitor listening socket for new connections
        FD_SET(server->listen_socket, &read_fds);
        
        // Monitor each client socket based on its state
        for (int i = 0; i < server->max_clients; i++) {
            ClientConnection *client = &server->clients[i];
            
            if (client->socket_fd == INVALID_SOCKET) {
                continue;  // Unused slot
            }
            
            // Readable: waiting for request or need to read more
            if (client->state == CLIENT_READING) {
                FD_SET(client->socket_fd, &read_fds);
            }
            
            // Writable: sending response
            if (client->state == CLIENT_WRITING) {
                FD_SET(client->socket_fd, &write_fds);
            }
        }
        
        // Windows select() with timeout
        int activity = select(0, &read_fds, &write_fds, NULL, &timeout);

        if (activity == SOCKET_ERROR) {
            printf("select() failed: %d\n", WSAGetLastError());
            break;
        }
        
        // Accept new connections
        if (FD_ISSET(server->listen_socket, &read_fds)) {
            ClientConnection *new_client = async_server_accept_client(server);
            if (new_client == NULL && server->num_clients >= server->max_clients) {
                server_log(server, "WARN", "Max clients (%d) reached, rejecting new connections",
                          server->max_clients);
            }
        }
        
        // Handle existing clients
        for (int i = 0; i < server->max_clients; i++) {
            ClientConnection *client = &server->clients[i];
            
            if (client->socket_fd == INVALID_SOCKET) {
                continue;
            }
            
            // Readable (has data to read)
            if (FD_ISSET(client->socket_fd, &read_fds)) {
                async_server_handle_client_read(server, client);
            }
            
            // Writable (socket buffer empty, ready to send)
            if (FD_ISSET(client->socket_fd, &write_fds)) {
                async_server_handle_client_write(server, client);
            }
            
            // Timeout detection
            time_t now = time(NULL);
            if ((now - client->connected_time) > server->config.request_timeout_seconds) {
                server_log(server, "WARN", "Client %s:%d timeout",
                          client->client_addr, client->client_port);
                async_server_close_client(server, client);
            }
        }
    }
    
    server_log(server, "INFO", "Event loop stopped");
    return 0;
}

// ============== CLIENT READ HANDLER ==============

void async_server_handle_client_read(AsyncServer *server, ClientConnection *client) {
    if (client->state == CLIENT_READING) {
        int result = parse_http_request(client);
        
        if (result == -1) {
            // Parse error or connection closed
            async_server_close_client(server, client);
            return;
        }
        
        if (result == 1) {
            // Full request received
            server->total_requests++;
            server_log(server, "INFO", "[%s] Received: %s %s (%d bytes body)",
                      client->request_id, client->method, client->path,
                      client->content_length);
            
            // For demo: just echo back the request
            char response[512];
            snprintf(response, sizeof(response),
                     "{\"request_id\": \"%s\", \"method\": \"%s\", \"path\": \"%s\", "
                     "\"body_length\": %d}", 
                     client->request_id, client->method, client->path,
                     client->content_length);
            
            build_http_response(client, response, 200);
            client->state = CLIENT_WRITING;
        }
    }
}

// ============== CLIENT WRITE HANDLER ==============

void async_server_handle_client_write(AsyncServer *server, ClientConnection *client) {
    if (client->state == CLIENT_WRITING) {
        // Send part of response
        int n = send(client->socket_fd,
                         client->response_buffer + client->response_sent,
                         (int)(client->response_total - client->response_sent), 0);
        
        if (n == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                printf("send() failed: %d\n", err);
                async_server_close_client(server, client);
            }
            return;
        }
        
        client->response_sent += n;
        
        if (client->response_sent >= client->response_total) {
            // Response fully sent
            server_log(server, "INFO", "[%s] Response sent (%zu bytes)",
                      client->request_id, client->response_total);
            async_server_close_client(server, client);
        }
    }
}

// ============== SHUTDOWN ==============

void async_server_stop(AsyncServer *server) {
    if (!server) return;
    server->running = 0;
}

void async_server_free(AsyncServer *server) {
    if (!server) return;
    
    if (server->listen_socket != INVALID_SOCKET) {
        closesocket(server->listen_socket);
    }
    
    for (int i = 0; i < server->max_clients; i++) {
        if (server->clients[i].socket_fd != INVALID_SOCKET) {
            async_server_close_client(server, &server->clients[i]);
        }
    }
    
    free(server->clients);
    queue_free(server->queue);
    free(server);
}

// ============== STATS ==============

void async_server_display_stats(AsyncServer *server) {
    if (!server) return;
    
    printf("\n═══════════════════════════════════════════════════════\n");
    printf("           Async Server Statistics\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    printf("Total requests:           %llu\n", server->total_requests);
    printf("Active clients:           %d/%d\n", server->num_clients, server->max_clients);
    printf("Peak concurrent:          %d\n", server->peak_concurrent_clients);
    printf("\n");
}