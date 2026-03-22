#ifndef NET_SOCKET_DEFS_H
#define NET_SOCKET_DEFS_H

#ifdef _WIN32
    #define _WINSOCKAPI_
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

#endif // NET_SOCKET_DEFS_H