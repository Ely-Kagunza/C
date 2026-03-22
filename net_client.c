#include "net_client.h"

NetworkClient* net_client_create(const char *host, int port)
{
    NetworkClient *client = malloc(sizeof(NetworkClient));

    client->socket = INVALID_SOCKET;
    client->server_host = host;
    client->server_port = port;
    client->is_connected = 0;
    client->queries_sent = 0;
    client->responses_received = 0;

    return client;
}

int net_client_connect(NetworkClient *client)
{
    if (client == NULL)
        return -1;

    printf("[CLIENT] Connecting to %s:%d\n", client->server_host, client->server_port);

    // In real implementation: socket() and connect() here
    // for demo: simulate connection success
    client->is_connected = 1;
    printf("[CLIENT] Connected to server\n\n");

    return 0;
}

int net_client_send_query(NetworkClient *client, const char *query)
{
    if (client == NULL || query == NULL)
        return -1;

    CONNECTION_CHECK(client->socket);

    if (!client->is_connected)
    {
        printf("[CLIENT ERROR] Not connected to server\n");
        return -1;
    }

    printf("[CLIENT] Sending query to server:\n");
    printf("         \"%s\"\n\n", query);

    client->queries_sent++;

    // In real implementation: serialize QueryRequest and send over socket
    return 0;
}

int net_client_receive_response(NetworkClient *client, QueryResponse *resp)
{
    if (client == NULL || resp == NULL)
        return -1;

    if (!client->is_connected)
    {
        printf("[CLIENT ERROR] Not connected to server\n");
        return -1;
    }

    printf("[CLIENT] Receiving response from server...\n");

    client->responses_received++;

    // In real implementation: receive and deserialize queryResponse
    return 0;
}

int net_client_disconnect(NetworkClient *client)
{
    if (client == NULL)
        return -1;

    if (!client->is_connected)
    {
        printf("[CLIENT ERROR] Already disconnected\n");
        return 0;
    }

    printf("[CLIENT] Disconnecting from server...\n");
    client->is_connected = 0;

    return 0;
}

void net_client_display_stats(NetworkClient *client)
{
    if (client == NULL)
        return;

    printf("\n=== Client Statistics ===\n");
    printf("  Server:              %s:%d\n", client->server_host, client->server_port);
    printf("  Connected:           %s\n", client->is_connected ? "Yes" : "No");
    printf("  Queries sent:        %d\n", client->queries_sent);
    printf("  Responses received:  %d\n\n", client->responses_received);
}

void net_client_free(NetworkClient *client)
{
    if (client == NULL)
        return;

    if (client->is_connected)
        net_client_disconnect(client);

    free(client);
}