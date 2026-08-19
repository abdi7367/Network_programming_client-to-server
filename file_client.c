/*
 * file_client.c - Part 4: File Sharing Client
 *
 * Connects to file_server.c, requests a file by name, and writes the
 * received bytes to disk under a new local filename (so it doesn't
 * overwrite the original if run on the same machine as the server).
 *
 * Build:   gcc -o file_client file_client.c
 * Run:     ./file_client <server_ip> <filename>
 * Example: ./file_client 127.0.0.1 notes.txt
 *          (test from other systems by replacing 127.0.0.1 with the
 *           server machine's LAN IP address, e.g. 192.168.1.10)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVER_PORT 7000
#define CHUNK_SIZE 4096
#define LINE_BUFFER_SIZE 256

/* Reads one line (up to \n) from the socket -- used to read the server's
 * initial "OK <size>" or "ERROR: ..." response line. */
void read_line(int sock_fd, char *out, size_t max_len)
{
    size_t idx = 0;
    char c;
    while (idx < max_len - 1)
    {
        ssize_t n = recv(sock_fd, &c, 1, 0);
        if (n <= 0 || c == '\n')
            break;
        out[idx++] = c;
    }
    out[idx] = '\0';
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <filename>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 notes.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    const char *filename = argv[2];

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid server IP address: %s\n", server_ip);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("Connected to file server at %s:%d\n", server_ip, SERVER_PORT);

    /* Send the requested filename, newline-terminated (matches server protocol). */
    char request[LINE_BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s\n", filename);
    send(sock_fd, request, strlen(request), 0);

    /* Read the server's response header line: "OK <size>" or "ERROR: ..." */
    char header[LINE_BUFFER_SIZE];
    read_line(sock_fd, header, sizeof(header));

    if (strncmp(header, "ERROR", 5) == 0)
    {
        fprintf(stderr, "Server error: %s\n", header);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    long file_size = 0;
    if (sscanf(header, "OK %ld", &file_size) != 1)
    {
        fprintf(stderr, "Unexpected server response: %s\n", header);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("Server reports file size: %ld bytes\n", file_size);

    /* Save under "received_<filename>" so we never overwrite an existing
     * local file with the same name, especially useful when testing
     * client and server on the same machine. */
    char out_path[LINE_BUFFER_SIZE + 16];
    snprintf(out_path, sizeof(out_path), "received_%s", filename);

    FILE *fp = fopen(out_path, "wb");
    if (fp == NULL)
    {
        perror("fopen (output file)");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    char chunk[CHUNK_SIZE];
    long total_received = 0;
    ssize_t bytes_received;

    while (total_received < file_size &&
           (bytes_received = recv(sock_fd, chunk, CHUNK_SIZE, 0)) > 0)
    {
        fwrite(chunk, 1, bytes_received, fp);
        total_received += bytes_received;
        printf("\rReceived %ld / %ld bytes", total_received, file_size);
        fflush(stdout);
    }
    printf("\n");

    fclose(fp);
    close(sock_fd);

    if (total_received == file_size)
    {
        printf("File transfer complete. Saved as \"%s\"\n", out_path);
    }
    else
    {
        printf("Warning: expected %ld bytes but received %ld bytes.\n",
               file_size, total_received);
    }

    return EXIT_SUCCESS;
}