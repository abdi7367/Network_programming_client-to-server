/*
 * http_server.c - Part 3: Simple HTTP Server
 *
 * A minimal HTTP/1.1 server that handles GET requests, returns a
 * static HTML response, and logs each request (method, URL, timestamp)
 * to http_requests.log.
 *
 * Build:   gcc -o http_server http_server.c
 * Run:     ./http_server
 * Test:    curl http://127.0.0.1:8080/
 *          or open http://127.0.0.1:8080/ in a browser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define LOG_FILE "http_requests.log"
#define YOUR_NAME "Abdullah" /* change if you want a different greeting */

/* Logs "METHOD URL - timestamp" to both stdout and the log file. */
void log_request(const char *method, const char *path)
{
    time_t now = time(NULL);
    char time_buf[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("[HTTP LOG] %s - %s %s\n", time_buf, method, path);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp != NULL)
    {
        fprintf(fp, "%s - %s %s\n", time_buf, method, path);
        fclose(fp);
    }
    else
    {
        perror("fopen (log file)");
    }
}

/* Builds and sends a full HTTP response (status line + headers + body). */
void send_response(int client_fd, const char *status, const char *body)
{
    char response[BUFFER_SIZE];
    int response_len = snprintf(response, sizeof(response),
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: %zu\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s",
                                status, strlen(body), body);

    send(client_fd, response, response_len, 0);
}

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP Server listening on http://127.0.0.1:%d/\n", PORT);

    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0)
        {
            close(client_fd);
            continue;
        }
        buffer[bytes_received] = '\0';

        /* Parse just the request line, e.g. "GET /path HTTP/1.1" */
        char method[16] = {0};
        char path[512] = {0};
        sscanf(buffer, "%15s %511s", method, path);

        log_request(method, path);

        if (strcmp(method, "GET") != 0)
        {
            /* Task only asks us to handle GET; reject anything else cleanly. */
            const char *body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
            send_response(client_fd, "405 Method Not Allowed", body);
        }
        else if (strstr(path, "..") != NULL)
        {
            /* Basic safety check against path traversal in the URL. */
            const char *body = "<html><body><h1>400 Bad Request</h1></body></html>";
            send_response(client_fd, "400 Bad Request", body);
        }
        else
        {
            char body[1024];
            snprintf(body, sizeof(body),
                     "<html><body><h1>Welcome %s!</h1>"
                     "<p>You requested: %s</p></body></html>",
                     YOUR_NAME, path);
            send_response(client_fd, "200 OK", body);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}