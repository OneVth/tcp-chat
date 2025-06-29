#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(25000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to echo server at %s:%d\n", "127.0.0.1", 25000);

    while (1)
    {
        printf("Client: ");
        if (fgets(buffer, 1024, stdin) == NULL)
            break;

        if (send(sockfd, buffer, strlen(buffer), 0) <= 0)
            break;

        ssize_t n = recv(sockfd, buffer, 1024 - 1, 0);
        if (n <= 0)
            break;

        buffer[n] = '\0';
        printf("Echoed: %s", buffer);
    }

    close(sockfd);
    return 0;
}