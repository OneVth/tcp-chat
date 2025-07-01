#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

constexpr int BUFFER_SIZE = 1024;
constexpr int PORT = 25000;

void *threadFunction(void *arg)
{
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t received = 0;

    std::cout << "New client connected: " << std::endl;

    while (true)
    {
        received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (received <= 0)
        {
            std::cout << "Client disconnected." << std::endl;
            close(client_fd);
            return nullptr;
        }

        buffer[received] = '\0';
        std::cout << "Received: " << buffer << std::endl;

        send(client_fd, buffer, received, 0);
    }
}

int main(void)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    // SO_REUSEADDR 설정
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "Echo server is running on port " << PORT << "..." << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};

    while (true)
    {
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        if (client_fd == -1)
        {
            perror("accept");
            close(server_fd);
            return EXIT_FAILURE;
        }

        pthread_t tid;
        if(pthread_create(&tid, nullptr, threadFunction, &client_fd) != 0)
        {
            perror("pthread_create");
            return EXIT_FAILURE;
        }

        pthread_detach(tid);
    }

    close(server_fd);

    return 0;
}