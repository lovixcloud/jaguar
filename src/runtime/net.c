#include "jag/runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define closesocket close
#endif

void jag_net_http_serve(int port, const char *response_body) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;
#endif

    int server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    if (listen(server_fd, 5) < 0) {
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    printf("[jaguar:http] Server listening on http://localhost:%d\n", port);

    int client_fd = (int)accept(server_fd, NULL, NULL);
    if (client_fd >= 0) {
        const char *body = response_body ? response_body : "Hello from Jaguar HTTP Server!";
        char http_resp[2048];
        snprintf(http_resp, sizeof(http_resp),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n\r\n"
                 "%s",
                 strlen(body), body);
        send(client_fd, http_resp, (int)strlen(http_resp), 0);
        closesocket(client_fd);
    }

    closesocket(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
}
