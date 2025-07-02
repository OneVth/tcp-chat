#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <arpa/inet.h>
#include <list>

constexpr int BUFFER_SIZE = 1024;
constexpr int PORT = 25000;

pthread_spinlock_t g_spin;
std::list<int> g_list_client;
int g_server_fd;

bool addUser(int sock_fd)
{
    pthread_spin_lock(&g_spin);
    g_list_client.push_back(sock_fd);
    pthread_spin_unlock(&g_spin);

    return true;
}

void send_chatting_message(char *param)
{
    int length = strlen(param);
    std::list<int>::iterator it;

    pthread_spin_lock(&g_spin);
    for (it = g_list_client.begin(); it != g_list_client.end(); it++)
        send(*it, param, sizeof(char) * (length + 1), 0);
    pthread_spin_unlock(&g_spin);
}

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
            pthread_spin_lock(&g_spin);
            g_list_client.remove(client_fd);
            pthread_spin_unlock(&g_spin);

            close(client_fd);
            return nullptr;
        }

        buffer[received] = '\0';
        std::cout << "Received: " << buffer << std::endl;

        send_chatting_message(buffer);
        memset(buffer, 0, BUFFER_SIZE);
    }
}

void signalHandler(int signum)
{
    std::list<int>::iterator it;

    std::cout << "\n[INFO] Interrupt signal (" << signum << ") received. Shutting down..." << std::endl;

    shutdown(g_server_fd, SHUT_RDWR);

    pthread_spin_lock(&g_spin);
    for (it = g_list_client.begin(); it != g_list_client.end(); it++)
        close(*it);

    g_list_client.clear();
    pthread_spin_unlock(&g_spin);

    sleep(1);
    pthread_spin_destroy(&g_spin);
    close(g_server_fd);

    exit(signum);
}

int main(void)
{
    pthread_spin_init(&g_spin, PTHREAD_PROCESS_PRIVATE);

    signal(SIGINT, signalHandler);

    int g_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_fd == -1)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    // SO_REUSEADDR 설정
    int opt = 1;
    if (setsockopt(g_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(g_server_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(g_server_fd);
        return EXIT_FAILURE;
    }

    if (listen(g_server_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(g_server_fd);
        return EXIT_FAILURE;
    }

    std::cout << "Echo server is running on port " << PORT << "..." << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};

    while (true)
    {
        int client_fd = accept(g_server_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        if (client_fd == -1)
        {
            perror("accept");
            close(g_server_fd);
            return EXIT_FAILURE;
        }

        if (addUser(client_fd) == false)
        {
            perror("addUser");
            break;
        }

        pthread_t tid;
        if (pthread_create(&tid, nullptr, threadFunction, &client_fd) != 0)
        {
            perror("pthread_create");
            return EXIT_FAILURE;
        }

        pthread_detach(tid);
    }

    std::cout << "Closing chatting server..." << std::endl;

    return 0;
}