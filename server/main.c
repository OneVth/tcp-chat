#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define PORT 25000

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // SO_REUSEADDR 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Echo server is running on port %d...\n", PORT);

    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1)
        {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        while (1)
        {
            ssize_t n = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (n <= 0)
            {
                printf("Client disconnected.\n");
                break;
            }

            buffer[n] = '\0';
            printf("Received: %s\n", buffer);
            fflush(stdout);

            send(client_fd, buffer, n, 0);
        }

        shutdown(server_fd, SHUT_RDWR);
        close(client_fd);
    }

    close(server_fd);

    return 0;
}