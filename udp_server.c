/*
 * udp_server.c - Part 2: UDP Messaging System
 *
 * A UDP server that listens on a port, receives short messages from
 * clients (no connection setup needed), prints them, and logs each
 * sender's IP and port.
 *
 * Build:   gcc -o udp_server udp_server.c
 * Run:     ./udp_server
 * Test:    echo -n "hello" | nc -u -w1 127.0.0.1 6000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 6000
#define BUFFER_SIZE 1024
#define LOG_FILE "connections.log"

void log_message(struct sockaddr_in *sender_addr, const char *msg)
{
    time_t now = time(NULL);
    char time_buf[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    /* inet_ntoa() is used here specifically since it's the API the task
     * asks us to explore for UDP; inet_ntop() is the modern equivalent
     * used in tcp_echo.c. Both are shown so you understand each. */
    char *ip_str = inet_ntoa(sender_addr->sin_addr);
    int sender_port = ntohs(sender_addr->sin_port);

    printf("[UDP LOG] %s - From %s:%d -> \"%s\"\n", time_buf, ip_str, sender_port, msg);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp != NULL)
    {
        fprintf(fp, "[UDP] %s - From %s:%d -> \"%s\"\n", time_buf, ip_str, sender_port, msg);
        fclose(fp);
    }
    else
    {
        perror("fopen (log file)");
    }
}

int main(void)
{
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    /* 1. Create a UDP socket (SOCK_DGRAM) — no connection state. */
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    /* 2. Bind so the OS knows which port delivers datagrams to us. */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("UDP Server listening on port %d...\n", PORT);

    /* 3. Loop forever: each recvfrom() call gets one datagram + sender info. */
    while (1)
    {
        ssize_t bytes_received = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0,
                                          (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0)
        {
            perror("recvfrom");
            continue;
        }

        buffer[bytes_received] = '\0';
        log_message(&client_addr, buffer);

        /* Optional: send a simple acknowledgement back to the sender. */
        const char *ack = "ACK: message received";
        sendto(server_fd, ack, strlen(ack), 0,
               (struct sockaddr *)&client_addr, client_len);
    }

    close(server_fd);
    return 0;
}