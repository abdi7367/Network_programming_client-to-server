/*
 * tcp_echo.c - Part 1: TCP Echo Server
 *
 * A TCP server that listens on a port, accepts a client connection,
 * echoes back whatever the client sends, and logs the client's
 * IP address and timestamp of each connection to connections.log.
 *
 * Build:   gcc -o tcp_echo tcp_echo.c
 * Run:     ./tcp_echo
 * Test:    In another terminal: nc 127.0.0.1 5000
 *          (or telnet 127.0.0.1 5000), then type a message and press Enter.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define LOG_FILE "connections.log"

/* Writes a line describing the client connection to both stdout and the log file. */
void log_connection(struct sockaddr_in *client_addr)
{
    time_t now = time(NULL);
    char time_buf[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, ip_str, sizeof(ip_str));
    int client_port = ntohs(client_addr->sin_port);

    printf("[LOG] %s - Client connected from %s:%d\n", time_buf, ip_str, client_port);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp != NULL)
    {
        fprintf(fp, "%s - Client connected from %s:%d\n", time_buf, ip_str, client_port);
        fclose(fp);
    }
    else
    {
        perror("fopen (log file)");
    }
}
/* Writes a line describing the client disconnection to both stdout and the log file. */
void log_disconnection(struct sockaddr_in *client_addr)
{
    time_t now = time(NULL);
    char time_buf[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, ip_str, sizeof(ip_str));
    int client_port = ntohs(client_addr->sin_port);

    printf("[LOG] %s - Client disconnected from %s:%d\n", time_buf, ip_str, client_port);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp != NULL)
    {
        fprintf(fp, "%s - Client disconnected from %s:%d\n", time_buf, ip_str, client_port);
        fclose(fp);
    }
    else
    {
        perror("fopen (log file)");
    }
}

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    /* 1. Create a TCP socket (SOCK_STREAM). */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Allow quick reuse of the port after restarting the server. */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* 2. Bind the socket to an address/port. */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; /* listen on all local interfaces */
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* 3. Listen for incoming connections. Backlog of 5 pending connections. */
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("TCP Echo Server listening on port %d...\n", PORT);

    /* Main server loop: accept one client at a time, echo, then repeat. */
    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue; /* don't crash the server on one bad accept */
        }

        /* Log IP + timestamp of this client (Part 1 requirement). */
        log_connection(&client_addr);

        /* Echo loop: keep echoing until the client disconnects or sends "exit". */
        ssize_t bytes_received;
        while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0)
        {
            buffer[bytes_received] = '\0';
            printf("Received: %s", buffer);

            if (strncmp(buffer, "exit", 4) == 0)
            {
                printf("Client requested exit.\n");
                break;
            }

            /* send() can write fewer bytes than requested, so loop until all sent. */
            ssize_t total_sent = 0;
            while (total_sent < bytes_received)
            {
                ssize_t sent = send(client_fd, buffer + total_sent,
                                    bytes_received - total_sent, 0);
                if (sent < 0)
                {
                    perror("send");
                    break;
                }
                total_sent += sent;
            }
        }

        if (bytes_received < 0)
        {
            perror("recv");
        }

        log_disconnection(&client_addr);
        close(client_fd);
    }

    close(server_fd); /* unreachable in this simple loop, but good practice */
    return 0;
}