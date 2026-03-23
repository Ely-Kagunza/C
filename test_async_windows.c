#define _CRT_SECURE_NO_WARNINGS
#include "async_server_windows.h"
#include <process.h>

DWORD WINAPI client_simulator(LPVOID arg) {
    int client_id = *(int *)arg;
    free(arg);

    Sleep(1000); // Wait for server

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("[Client %d] socket() failed: %d\n", client_id, WSAGetLastError());
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Client %d] connect() failed: %d\n", client_id, WSAGetLastError());
        closesocket(sock);
        return 1;
    }

    const char *request = "POST /v1/infer HTTP/1.1\r\n"
                         "Host: localhost:8080\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: 13\r\n"
                         "\r\n"
                         "{\"value\": 42}";

    if (send(sock, request, (int)strlen(request), 0) == SOCKET_ERROR) {
        printf("[Client %d] send() failed: %d\n", client_id, WSAGetLastError());
        closesocket(sock);
        return 1;
    }

    char buffer[4096];
    int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("[Client %d] Got response (%d bytes\n)", client_id, n);
    }

    closesocket(sock);
    return 0;
}

int main() {
    // Initialize Winsock (REQUIRED FOR WINDOWS)
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    printf("═══════════════════════════════════════════════════════\n");
    printf("    Phase 14: Event-Driven Async Server (Windows)\n");
    printf("═══════════════════════════════════════════════════════\n\n");

    ServerConfig config = {
        .port = 8080,
        .max_clients = 100,
        .queue_size = 1000,
        .request_timeout_seconds = 30,
        .enable_logging = 1
    };

    AsyncServer *server = async_server_create(config);
    if (!server) {
        printf("Failed to create async server\n");
        WSACleanup();
        return 1;
    }

    if (async_server_start(server, "127.0.0.1") == -1) {
        printf("Failed to start async server\n");
        async_server_free(server);
        WSACleanup();
        return 1;
    }

    printf("\nServer started. Spawning 5 test clients...\n\n");

    // Create threads using windows API
    HANDLE threads[5];
    for (int i = 0; i < 5; i++) {
        int *client_id = malloc(sizeof(int));
        *client_id = i + 1;
        threads[i] = CreateThread(NULL, 0, client_simulator, client_id, 0, NULL);
    }

    // Run server in background thread
    HANDLE server_thread = CreateThread(NULL, 0, 
        (LPTHREAD_START_ROUTINE)async_server_run, 
        server, 0, NULL);

    // Run server for ~40 seconds (allows 5 client threads + timeout)
    Sleep(40000);
    async_server_stop(server);

    // Wait for server thread to finish
    WaitForSingleObject(server_thread, INFINITE);
    CloseHandle(server_thread);

    // Wait for all threads to complete
    WaitForMultipleObjects(5, threads, TRUE, INFINITE);for (int i = 0; i < 5; i++) {
        CloseHandle(threads[i]);
    }

    async_server_display_stats(server);
    async_server_free(server);

    printf("\nTest complete!\n");

    WSACleanup(); // Clean up Winsock
    return 0;
}