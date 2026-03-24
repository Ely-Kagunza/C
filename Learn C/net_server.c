#include "net_server.h"

// ============== TCP SERVER DATA =============

typedef struct
{
    SOCKET client_sockets[100];
    int client_count;
    int queries_received;
    int responses_sent;
} TCPServerData;

// =============== TCP SERVER IMPLEMENTATION ==============

int tcp_accept_client(NetworkServer *self)
{
    TCPServerData *data = (TCPServerData *)self->strategy_data;

    if (data->client_count >= self->max_clients)
    {
        printf("[SERVER] Max clients reached\n");
        return -1;
    }

    // In real implementation: accept() call here
    // For demo: simulate client connection
    int client_id = data->client_count++;
    printf("[SERVER] Client %d connected\n", client_id);

    if (self->callbacks.on_connected)
        self->callbacks.on_connected(client_id, "127.0.0.1", self->callbacks.user_data);

    return client_id;
}

int tcp_handle_query(NetworkServer *self, int client_id, const char *query)
{
    TCPServerData *data = (TCPServerData *)self->strategy_data;
    Database *db = self->db;

    data->queries_received++;

    printf("[SERVER] Received query from client %d: %s\n", client_id, query);

    if (self->callbacks.on_query_received)
        self->callbacks.on_query_received(client_id, query, self->callbacks.user_data);

    // In real implementation: parse and execute SQL query
    // For demo: return first 5 records
    printf("[SERVER] Executing query...\n");

    return 0;
}

int tcp_send_response(NetworkServer *self, int client_id, QueryResponse *resp)
{
    TCPServerData *data = (TCPServerData *)self->strategy_data;

    data->responses_sent++;

    printf("[SERVER] Sending %d records to client %d\n", resp->header.record_count, client_id);

    if (self->callbacks.on_response_sent)
        self->callbacks.on_response_sent(client_id, resp->header.record_count, self->callbacks.user_data);

    // In real implementation: serialize and send over socket
    return 0;
}

void tcp_display_stats(NetworkServer *self)
{
    TCPServerData *data = (TCPServerData *)self->strategy_data;

    printf("\n=== TCP Server Statistics ===\n");
    printf("  Port:                %d\n", self->port);
    printf("  Active clients:      %d/%d\n", data->client_count, self->max_clients);
    printf("  Queries received:    %d\n", data->queries_received);
    printf("  Responses sent:      %d\n\n", data->responses_sent);
}

void tcp_free_server(NetworkServer *self)
{
    free(self->strategy_data);
    free(self);
}

NetworkServer *net_server_tcp_create(Database *db, int port, NetworkCallbacks callbacks)
{
    NetworkServer *server = malloc(sizeof(NetworkServer));
    TCPServerData *data = malloc(sizeof(TCPServerData));

    memset(data, 0, sizeof(TCPServerData));

    server->listen_socket = INVALID_SOCKET;
    server->port = port;
    server->max_clients = 100;
    server->active_clients = 0;
    server->accept_client = tcp_accept_client;
    server->handle_client_query = tcp_handle_query;
    server->send_response = tcp_send_response;
    server->display_stats = tcp_display_stats;
    server->free_server = tcp_free_server;
    server->name = "TCP";
    server->db = db;
    server->callbacks = callbacks;
    server->strategy_data = data;

    return server;
}

// =============== GENERIC SERVER FUNCTIONS ==============

int net_server_start(NetworkServer *server)
{
    if (server == NULL)
    {
        printf("[ERROR] Server is NULL\n");
        return -1;
    }

    printf("\n═══════════════════════════════════════════════════════\n");
    printf("Starting %s Network Server on port %d\n", server->name, server->port);
    printf("═══════════════════════════════════════════════════════\n\n");

    printf("[SERVER] Listening for client connections...\n");
    printf("[SERVER] Ready to accept queries\n\n");

    return 0;
}

int net_server_stop(NetworkServer *server)
{
    if (server == NULL)
        return -1;

    printf("[SERVER] Shutting down...\n");
    server->display_stats(server);

    return 0;
}