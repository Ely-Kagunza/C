#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net_socket_defs.h"
#include "net_protocol.h"

// ============== CLIENT ERROR HANDLING MACROS (Phase 11 Part 3) ==============

#define CLIENT_CALL(call) \
    do { \
        if ((call) == SOCKET_ERROR) { \
            printf("[CLIENT ERROR] " #call " failed\n"); \
            return -1; \
        } \
    } while (0)

#define CONNECTION_CHECK(sock) \
    do { \
        if ((sock) == INVALID_SOCKET) { \
            printf("[CLIENT ERROR] Not connected to server\n"); \
            return -1; \
        } \
    } while (0)

// ============== NETWORK CLIENT ===============

typedef struct {
    SOCKET socket;
    const char *server_host;
    int server_port;
    int is_connected;
    int queries_sent;
    int responses_received;
} NetworkClient;

// ============== CLIENT OPERATIONS ===============

// Create client
NetworkClient* net_client_create(const char *host, int port);

// Connect to server
int net_client_connect(NetworkClient *client);

// Send query to server
int net_client_send_query(NetworkClient *client, const char *query);

// Receive response from server
int net_client_receive_response(NetworkClient *client, QueryResponse *resp);

// Disconnect from server
int net_client_disconnect(NetworkClient *client);

// Get statistics
void net_client_display_stats(NetworkClient *client);

// Free client
void net_client_free(NetworkClient *client);

#endif // NET_CLIENT_H