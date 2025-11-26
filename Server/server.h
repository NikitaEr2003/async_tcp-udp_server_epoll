#pragma once
#include <cstddef>
#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "threadpool.h"
#include <unordered_map>

struct ClientInfo // Структура для хранения данный о tcp клиенте
{
    int client_fd_;
    sockaddr_in client_addr_;
};

class Server
{
    static const int MAX_EVENTS_ = 64;    // Максимальное число событий
    static const int BUFFER_SIZE_ = 1024; // Буффер сообщения

public:
    Server(std::atomic<bool> & running); // Флаг для остановки
    ~Server();

    bool start(int port, size_t count_threads); // Делаем начальную иницилизацию
    void run();                                 // Запускаем наш epoll в режим ожидания событий

    void stop(); // Завершаем работу сервера

private:
    int socket_tcp_; // Дескриптоор tcp
    int socket_udp_; // Дескриптор udp
    int epoll_fd_;   // Дескриптор epoll
    int wake_up_fd;  // Дескриптор для пробуждения epoll

    std::atomic<size_t> total_clients_ = 0;   // Общее количество подключенных клиентов
    std::atomic<size_t> current_clients_ = 0; // Текущий онлайн
    std::atomic<bool> &running_;              // Флаг работы
    std::atomic<bool> shutdown_{false}; // Флаг shutdown

    std::unordered_map<int, ClientInfo> clients_; // Мапа для хранения данных о tcp клиентах
    std::mutex clients_mutex_;
    std::unique_ptr<Threadpool> thread_pool; // Наш пул потоков

    void closeClient(int client); // Удаляем клиента из epoll и из map

    std::string getStats();       // Получаем статистику
    std::string getCurrentTime(); // Получаем текущее время

    void handleTcpConnection();           // Обработка tcp подключения
    void handleUdpMessage();              // Обработка udp сообщения
    void handleTcpMessage(int client_fd); // Обработка tcp сообщения

    std::string processMessage(const std::string &message);                                     // Обрабатываем какое именно сообщение нам пришло
    void processTcpMessage(int client_fd, sockaddr_in client_addr, const std::string &message); // Отвечаем tcp сообщением
    void processUdpMessage(sockaddr_in client_addr, socklen_t len, const std::string &message); // Отвечаем udp сообщением
};