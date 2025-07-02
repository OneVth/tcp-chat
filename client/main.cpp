#include <pthread.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

constexpr int BUFFER_SIZE = 1024;
constexpr int PORT = 25000;

void *threadReceive(void *arg)
{
    int server_fd = *(int *)arg;
    char buffer[BUFFER_SIZE] = {0};

    while (true)
    {
        ssize_t received = recv(server_fd, buffer, BUFFER_SIZE - 1, 0);
        if (received <= 0)
        {
            std::cout << "recv() thread ended" << std::endl;
            return nullptr;
        }

        buffer[received] = '\0';
        std::cout << "-> " << buffer << std::endl;
        memset(buffer, 0, BUFFER_SIZE);
    }
}

int main(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    std::cout << "Connected to echo server at " << "127.0.0.1" << ":" << PORT << std::endl;

    pthread_t tid;
    if (pthread_create(&tid, nullptr, threadReceive, &sockfd) != 0)
    {
        perror("pthread_create");
        return EXIT_FAILURE;
    }
    pthread_detach(tid);

    std::cout << "Chatting server opened." << std::endl;
    while (true)
    {
        char buffer[BUFFER_SIZE] = {0};
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
            break;
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "EXIT") == 0)
            break;

        if (send(sockfd, buffer, strlen(buffer), 0) <= 0)
            break;
    }

    close(sockfd);
    return 0;
}