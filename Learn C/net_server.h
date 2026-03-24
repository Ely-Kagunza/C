#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net_socket_defs.h"
#include "database.h"
#include "net_protocol.h"

// ============== NETWORK ERROR HANDLING MACROS (Phase 11 Part 3) ==============

#define NETWORK_CALL(call)                         \
    do                                             \
    {                                              \
        if ((call) == SOCKET_ERROR)                \
        {                                          \
            perror("Network call failed: " #call); \
            return -1;                             \
        }                                          \
    } while (0)

#define SOCKET_ERROR_CHECK(sock)                \
    do                                          \
    {                                           \
        if ((sock) == INVALID_SOCKET)           \
        {                                       \
            printf("[ERROR] Invalid socket\n"); \
            return NULL;                        \
        }                                       \
    } while (0)

// ============== NETWORK CALLBACKS (Phase 11 Part 1) ==============

typedef void (*OnClientConnectedCallback)(int client_id, const char *client_addr, void *user_data);
typedef void (*OnQueryReceivedCallback)(int client_id, const char *query, void *user_data);
typedef void (*OnResponseSentCallback)(int client_id, int record_count, void *user_data);
typedef void (*OnClientDisconnectedCallback)(int client_id, void *user_data);

typedef struct
{
    OnClientConnectedCallback on_connected;
    OnQueryReceivedCallback on_query_received;
    OnResponseSentCallback on_response_sent;
    OnClientDisconnectedCallback on_disconnected;
    void *user_data;
} NetworkCallbacks;

// ============== NETWORK SERVER STRUCTURE (Phase 11 Part 2) ==============

typedef struct NetworkServer
{
    SOCKET listen_socket;
    int port;
    int max_clients;
    int active_clients;

    // Vtable functions
    int (*accept_client)(struct NetworkServer *self);
    int (*handle_client_query)(struct NetworkServer *self, int client_id, const char *query);
    int (*send_response)(struct NetworkServer *self, int client_id, QueryResponse *resp);
    void (*display_stats)(struct NetworkServer *self);
    void (*free_server)(struct NetworkServer *self);

    // Metadata
    const char *name;
    Database *db;
    NetworkCallbacks callbacks;
    void *strategy_data;
} NetworkServer;

// ============== SERVER IMPLEMENTATIONS ==============

// TCP Server (standard implimentation)
NetworkServer *net_server_tcp_create(Database *db, int port, NetworkCallbacks callbacks);

// Start server (listen for connections)
int net_server_start(NetworkServer *server);

// Stop server
int net_server_stop(NetworkServer *server);

#endif // NET_SERVER_H