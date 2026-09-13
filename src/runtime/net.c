#include "jag/runtime.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void jag_net_http_serve(int port, const char *response_body) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        return;
    }

    printf("[jaguar:http] Server listening on http://localhost:%d\n", port);

    int client_fd = accept(server_fd, NULL, NULL);
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
        send(client_fd, http_resp, strlen(http_resp), 0);
        close(client_fd);
    }

    close(server_fd);
}
