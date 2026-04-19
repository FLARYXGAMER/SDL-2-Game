#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

void testServerFun()
{
    int server_fd, client_fd;
    struct sockaddr_in addr;
    char buffer[1024] = {0};
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    printf("Server: waiting for connection on port %d...\n", PORT);

    client_fd = accept(server_fd, NULL, NULL);
    read(client_fd, buffer, sizeof(buffer));
    printf("Server received: %s\n", buffer);
    send(client_fd, "Hello from server!", 18, 0);

    close(client_fd);
    close(server_fd);
}

void testClientFun()
{
    int sock;
    struct sockaddr_in addr;
    char buffer[1024] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        printf("Client: connection failed\n");
        close(sock);
        return;
    }

    printf("Client: connected!\n");
    send(sock, "Hello from client!", 18, 0);
    read(sock, buffer, sizeof(buffer));
    printf("Client received: %s\n", buffer);

    close(sock);
}
void* serverThread(void* arg)
{
    testServerFun(NULL);
    return NULL;
}