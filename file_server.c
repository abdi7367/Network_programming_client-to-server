/*
 * file_server.c - Part 4: File Sharing Server
 *
 * A TCP server that accepts a filename from a client, reads that file
 * from disk, and sends its contents back in fixed-size chunks. Works
 * with multiple clients (one at a time, sequential loop — select()
 * would be needed for true concurrency, noted as optional in the task).
 *
 * Protocol (simple, text + binary):
 *   1. Client connects and sends the filename, e.g. "notes.txt\n"
 *   2. Server checks if file exists:
 *        - If not found: sends "ERROR: File not found\n" and closes.
 *        - If found: sends "OK <filesize>\n" then streams the raw
 *          file bytes in CHUNK_SIZE pieces.
 *
 * Build:   gcc -o file_server file_server.c
 * Run:     ./file_server
 *          (make sure the files you want to share are in the same
 *           directory as the server, or adjust the path)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define PORT 7000
#define CHUNK_SIZE 4096
#define NAME_BUFFER_SIZE 256
#define LOG_FILE "connections.log"

void log_transfer(struct sockaddr_in *client_addr, const char *filename, const char *result)
{
    time_t now = time(NULL);
    char time_buf[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, ip_str, sizeof(ip_str));

    printf("[FILE LOG] %s - %s requested \"%s\" -> %s\n", time_buf, ip_str, filename, result);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp != NULL)
    {
        fprintf(fp, "[FILE] %s - %s requested \"%s\" -> %s\n", time_buf, ip_str, filename, result);
        fclose(fp);
    }
}

/* Reads the requested filename from the client (up to newline or buffer size). */
void read_filename(int client_fd, char *filename, size_t max_len)
{
    size_t idx = 0;
    char c;
    while (idx < max_len - 1)
    {
        ssize_t n = recv(client_fd, &c, 1, 0);
        if (n <= 0 || c == '\n')
            break;
        filename[idx++] = c;
    }
    filename[idx] = '\0';
}

/* Handles one client: reads filename, sends the file (or an error). */
void handle_client(int client_fd, struct sockaddr_in *client_addr)
{
    char filename[NAME_BUFFER_SIZE];
    read_filename(client_fd, filename, sizeof(filename));

    struct stat st;
    if (stat(filename, &st) != 0)
    {
        const char *err = "ERROR: File not found\n";
        send(client_fd, err, strlen(err), 0);
        log_transfer(client_addr, filename, "NOT FOUND");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        const char *err = "ERROR: Could not open file\n";
        send(client_fd, err, strlen(err), 0);
        log_transfer(client_addr, filename, "OPEN FAILED");
        return;
    }

    /* Tell the client "OK <size>" so it knows exactly how many bytes to expect. */
    char header[64];
    int header_len = snprintf(header, sizeof(header), "OK %ld\n", (long)st.st_size);
    send(client_fd, header, header_len, 0);

    char chunk[CHUNK_SIZE];
    size_t bytes_read;
    long total_sent = 0;

    while ((bytes_read = fread(chunk, 1, CHUNK_SIZE, fp)) > 0)
    {
        size_t total_written = 0;
        while (total_written < bytes_read)
        {
            ssize_t sent = send(client_fd, chunk + total_written, bytes_read - total_written, 0);
            if (sent < 0)
            {
                perror("send");
                fclose(fp);
                log_transfer(client_addr, filename, "SEND ERROR");
                return;
            }
            total_written += sent;
        }
        total_sent += bytes_read;
    }

    fclose(fp);
    printf("Sent %ld bytes for file \"%s\"\n", total_sent, filename);
    log_transfer(client_addr, filename, "SUCCESS");
}

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

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

    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("File Server listening on port %d...\n", PORT);
    printf("Serving files from the current directory: %s\n", "./");

    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        printf("Client connected: %s\n", ip_str);

        handle_client(client_fd, &client_addr);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}