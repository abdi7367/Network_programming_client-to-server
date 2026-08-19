# Network_programming_client-to-server

## Task Number 4: Network Programming and File Sharing in C

## 1. Project Overview

This project demonstrates practical network programming in C using TCP, UDP, HTTP, and TCP-based file sharing. The programs use the POSIX socket API and were compiled and tested with GCC inside Ubuntu/WSL.

The four parts are:

1. TCP echo server on port `5000`.
2. UDP messaging server on port `6000`.
3. Simple HTTP server on port `8080`.
4. TCP file server and client on port `7000`.

The project demonstrates socket creation, address binding, client/server communication, data transmission, logging, HTTP request handling, and file transfer between systems.

## 2. Project Files

| File | Purpose |
|---|---|
| `tcp_echo.c` | TCP server that echoes messages and logs connections. |
| `udp_server.c` | UDP server that logs messages and sends acknowledgments. |
| `http_server.c` | HTTP server that handles basic GET requests and returns HTML. |
| `file_server.c` | TCP server that receives a filename and sends the requested file. |
| `file_client.c` | TCP client that requests a file and saves the received data. |
| `bigmsg.txt` | Test file used for file transfer. |
| `connections.log` | TCP, UDP, and file-transfer log output. |
| `http_requests.log` | HTTP method, path, and timestamp log output. |
| `received_bigmsg.txt` | Copy of `bigmsg.txt` created by the file client. |

## 3. Development Environment

The programs use Linux/POSIX headers and functions:

```c
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
```

They should be compiled in Ubuntu, WSL, or another Linux environment. The Windows `cl.exe` task cannot compile them by default because Windows does not provide these POSIX headers.

## 4. Build Instructions

From Windows WSL:

```bash
cd /mnt/d/cyber/socket_task#4
gcc -Wall -Wextra -pedantic -o tcp_echo tcp_echo.c
gcc -Wall -Wextra -pedantic -o udp_server udp_server.c
gcc -Wall -Wextra -pedantic -o http_server http_server.c
gcc -Wall -Wextra -pedantic -o file_server file_server.c
gcc -Wall -Wextra -pedantic -o file_client file_client.c
```

From the Ubuntu VMware project folder:

```bash
cd ~/socket_task4
gcc -Wall -Wextra -pedantic -o file_server file_server.c
gcc -Wall -Wextra -pedantic -o file_client file_client.c
```

The source files compiled successfully with GCC and the warning options above.

## 5. Part 1: TCP Echo Server

### Objective

`tcp_echo.c` listens for a TCP connection, receives data, and sends the same data back. It records the client's IP address, port, and timestamps in `connections.log`.

### Main APIs

- `socket(AF_INET, SOCK_STREAM, 0)` creates a TCP socket.
- `setsockopt()` enables quick reuse of the port.
- `bind()` assigns port `5000`.
- `listen()` waits for connections.
- `accept()` creates a connected client socket.
- `recv()` receives client data.
- `send()` sends data back.
- `inet_ntop()` converts the address to readable text.
- `time()` and `strftime()` create timestamps.

### Test

Terminal 1:

```bash
cd /mnt/d/cyber/socket_task#4
./tcp_echo
```

Terminal 2:

```bash
printf "Hello from TCP\n" | nc -N 127.0.0.1 5000
```

Expected client output:

```text
Hello from TCP
```

The `-N` option closes the netcat connection after sending. Without it, netcat may remain connected and wait for more data.

Test the special exit behavior with:

```bash
printf "exit\n" | nc -N 127.0.0.1 5000
```

The server prints `Client requested exit.` and closes that client connection. It intentionally does not echo the word `exit`.

Example server evidence:

```text
TCP Echo Server listening on port 5000...
[LOG] 2026-08-19 00:39:32 - Client connected from 127.0.0.1:36486
Received: Hello from TCP
[LOG] 2026-08-19 00:39:34 - Client disconnected from 127.0.0.1:36486
```

## 6. Part 2: UDP Messaging System

### Objective

`udp_server.c` receives independent UDP datagrams. UDP does not establish a connection before sending data. The server logs each message with the sender's IP and port, then sends an acknowledgment.

### Main APIs

- `socket(AF_INET, SOCK_DGRAM, 0)` creates a UDP socket.
- `bind()` assigns port `6000`.
- `recvfrom()` receives a datagram and sender address.
- `sendto()` sends an acknowledgment.
- `inet_ntoa()` converts the sender address to readable text.

### Test

Terminal 1:

```bash
cd /mnt/d/cyber/socket_task#4
./udp_server
```

Terminal 2:

```bash
printf "Hello from UDP" | nc -u -w 1 127.0.0.1 6000
printf "Second UDP message" | nc -u -w 1 127.0.0.1 6000
```

Example server output:

```text
UDP Server listening on port 6000...
[UDP LOG] 2026-08-19 00:49:27 - From 127.0.0.1:51192 -> "Hello from UDP"
[UDP LOG] 2026-08-19 00:49:38 - From 127.0.0.1:46690 -> "Second UDP message"
```

Netcat acts as the test client. The server sends `ACK: message received` to the sender.

## 7. Part 3: Simple HTTP Server

### Objective

`http_server.c` listens on port `8080`, reads an HTTP request, extracts the method and URL path, logs them, and returns basic HTML.

### Main APIs

- `recv()` reads the HTTP request.
- `sscanf()` extracts the method and path.
- `strcmp()` checks whether the method is `GET`.
- `strstr()` checks for a path traversal sequence.
- `snprintf()` builds the HTTP response.
- `send()` sends the response.

### Test

Terminal 1:

```bash
cd /mnt/d/cyber/socket_task#4
./http_server
```

Terminal 2:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/test
```

Expected response structure:

```text
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: ...
Connection: close

<html><body><h1>Welcome Abdullah!</h1><p>You requested: /</p></body></html>
```

Requests are appended to `http_requests.log` with the method, path, and time. The greeting name is controlled by `YOUR_NAME` in `http_server.c`.

## 8. Part 4: File Sharing

### Objective

The file-sharing system uses TCP. The client sends a newline-terminated filename. The server checks the file, sends a header containing the file size, and streams the file in chunks. The client receives the exact number of bytes and saves them to a new local file.

### Protocol

The client sends:

```text
bigmsg.txt\n
```

For an existing file, the server sends a header such as:

```text
OK 5001\n
```

The header is followed by the raw file bytes. For a missing file, it sends:

```text
ERROR: File not found\n
```

The client saves the result using the `received_` prefix, so it does not overwrite the original file.

### Main APIs

- `fopen()` opens the requested file.
- `stat()` obtains its size.
- `fread()` reads file data into a buffer.
- `send()` transfers data in chunks.
- `recv()` receives the header and file data.
- `fwrite()` saves received bytes.

### Same-computer test

In Terminal 1:

```bash
cd /mnt/d/cyber/socket_task#4
./file_server
```

In Terminal 2:

```bash
cd /mnt/d/cyber/socket_task#4
./file_client 127.0.0.1 bigmsg.txt
```

Here, `127.0.0.1` means the same computer. The server and client are separate processes running in two terminals.

### VMware cross-machine test

The actual cross-machine test used:

- Ubuntu VMware as the file server.
- Windows WSL as the file client.
- Ubuntu server address: `192.168.100.10`.
- Server port: `7000`.

In Ubuntu VMware:

```bash
cd ~/socket_task4
./file_server
```

In Windows WSL:

```bash
cd /mnt/d/cyber/socket_task#4
./file_client 192.168.100.10 bigmsg.txt
```

Verified runtime result:

```text
Connected to file server at 192.168.100.10:7000
Server reports file size: 5001 bytes
Received 5001 / 5001 bytes
File transfer complete. Saved as "received_bigmsg.txt"
```

This proves that the Windows WSL client connected to the Ubuntu VM over the network and received the complete 5001-byte file.

### Verify file integrity

On the client machine:

```bash
cmp bigmsg.txt received_bigmsg.txt && echo "Files match successfully"
```

Expected output:

```text
Files match successfully
```

An additional check is:

```bash
sha256sum bigmsg.txt received_bigmsg.txt
```

Both hashes must be identical.

### Test a missing file

```bash
./file_client 192.168.100.10 missing.txt
```

Expected output:

```text
Connected to file server at 192.168.100.10:7000
Server error: ERROR: File not found
```

The server records successful and failed requests in `connections.log`.

## 9. Logging

The programs open their log files in append mode, so new events are added without deleting previous events.

View the TCP, UDP, and file-transfer log from the directory where the server was started:

```bash
cat connections.log
```

Example entries from the project log:

```text
[FILE] 2026-08-19 00:56:08 - 127.0.0.1 requested "bigmsg.txt" -> SUCCESS
[FILE] 2026-08-19 00:58:12 - 127.0.0.1 requested "missing.txt" -> NOT FOUND
```

For the Ubuntu VMware server:

```bash
cd ~/socket_task4
cat connections.log
```

View HTTP requests with:

```bash
cat http_requests.log
```

## 10. Runtime Evidence Checklist

Recommended screenshots or terminal captures:

- Successful GCC compilation.
- TCP server echoing a message.
- TCP connection and disconnection logs.
- UDP messages with sender IP and port.
- HTTP `curl -i` response showing `200 OK` and HTML.
- HTTP request log containing method and path.
- Ubuntu VM file server running.
- Windows WSL client showing the 5001-byte transfer.
- `cmp` output showing `Files match successfully`.
- Missing-file test showing `ERROR: File not found`.
- Server log showing `SUCCESS` and `NOT FOUND`.

## 11. Troubleshooting

### POSIX headers cannot be found

Errors for `arpa/inet.h`, `netinet/in.h`, or `sys/socket.h` mean that the file is being analyzed with Windows tooling. Open the project in WSL and compile it with GCC:

```bash
cd /mnt/d/cyber/socket_task#4
gcc -Wall -Wextra -pedantic -o tcp_echo tcp_echo.c
```

### Source file cannot be found

Use the correct directory. In Ubuntu VMware:

```bash
cd ~/socket_task4
```

In Windows WSL:

```bash
cd /mnt/d/cyber/socket_task#4
```

### TCP netcat appears to wait

Use `-N` to close the connection after sending:

```bash
printf "Hello from TCP\n" | nc -N 127.0.0.1 5000
```

If `-N` is unavailable, use:

```bash
printf "Hello from TCP\n" | nc -q 1 127.0.0.1 5000
```

### VMware client cannot connect

Find the Ubuntu address with:

```bash
hostname -I
```

Ensure VMware networking permits host-to-guest connections. If UFW is active, allow the file-server port:

```bash
sudo ufw allow 7000/tcp
```

Test the port from WSL:

```bash
nc -vz 192.168.100.10 7000
```

## 12. Limitations and Learning Outcomes

The file server processes clients sequentially. It accepts one client, serves that request, closes the connection, and then accepts the next client. The assignment marks `select()` as optional; simultaneous clients would require `select()`, threads, or separate processes.

The project demonstrates:

- The difference between TCP streams and UDP datagrams.
- The TCP lifecycle: socket, bind, listen, accept, receive, send, and close.
- How IP addresses and ports identify network endpoints.
- The structure of an HTTP request and response.
- How a text header can describe following binary file data.
- How partial TCP sends are handled during file transfer.
- How log files provide runtime evidence.

## 13. Conclusion

The required programs were compiled with GCC and tested at runtime. TCP echo communication, UDP messaging, HTTP GET handling, and TCP file sharing were demonstrated. The file-sharing test was completed across Windows WSL and Ubuntu VMware, and the client received all 5001 bytes of `bigmsg.txt` successfully.
