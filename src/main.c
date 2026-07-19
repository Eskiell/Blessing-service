/*
 * ps5-store - homebrew "library/store" payload
 *
 * Step 1+2: bare-bones background payload that opens a raw TCP socket
 * and serves a single static HTML page. This is the skeleton the PS5
 * home-screen launcher tile will point to (http://127.0.0.1:PORT/).
 *
 * No external HTTP library on purpose: PS5 payloads run in a
 * constrained sandbox and every extra dependency is one more thing
 * that can fail to link/run. Plain BSD sockets keep this predictable.
 *
 * Build: see Makefile (requires PS5_PAYLOAD_SDK).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LISTEN_PORT 5911
#define BACKLOG     8
#define REQ_BUF_SIZE 4096

static const char *HTML_PAGE =
    "<!DOCTYPE html>\n"
    "<html><head><meta charset=\"utf-8\"><title>Minha Loja PS5</title>\n"
    "<style>\n"
    "  body{background:#0d0d0d;color:#eee;font-family:sans-serif;\n"
    "       display:flex;align-items:center;justify-content:center;\n"
    "       height:100vh;margin:0}\n"
    "  h1{font-size:2.5em}\n"
    "</style></head>\n"
    "<body><h1>Minha Loja PS5</h1></body></html>\n";

/* Reads (and discards) the request, then writes back a fixed HTML response. */
static void handle_client(int client_fd) {
    char req[REQ_BUF_SIZE];
    ssize_t n = recv(client_fd, req, sizeof(req) - 1, 0);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    req[n] = '\0'; /* not parsed yet - step 3+ will add routing */

    char header[256];
    int body_len = (int)strlen(HTML_PAGE);
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n", body_len);

    send(client_fd, header, header_len, 0);
    send(client_fd, HTML_PAGE, body_len, 0);
    close(client_fd);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN); /* a client closing mid-write shouldn't kill us */

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LISTEN_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("[ps5-store] listening on port %d\n", LISTEN_PORT);

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            continue; /* accept can be interrupted, just retry */
        }
        handle_client(client_fd);
    }

    close(server_fd); /* unreachable, kept for clarity */
    return 0;
}
