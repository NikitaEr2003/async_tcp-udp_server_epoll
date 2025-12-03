#include "server.h"
#include "threadpool.h"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/eventfd.h>

Server::Server(std::atomic<bool> &running) : running_(running)
{
}

bool Server::start(int port, size_t count_threads)
{
  thread_pool = std::make_unique<Threadpool>(count_threads);

  socket_tcp_ = socket(AF_INET, SOCK_STREAM, 0);
  socket_udp_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_tcp_ == -1 || socket_udp_ == -1)
  {
    perror("invalid sockets: ");
    return false;
  }
  int opt = 1;
  if (setsockopt(socket_tcp_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) == -1)
  {
    perror("setsockop tcp: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }

  if (setsockopt(socket_udp_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) == -1)
  {
    perror("setsockop udp: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }

  struct sockaddr_in address_;
  memset(&address_, 0, sizeof(address_));
  address_.sin_family = AF_INET;
  address_.sin_port = htons(port);
  address_.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (bind(socket_tcp_, (struct sockaddr *)(&address_), sizeof(address_)) ==
      -1)
  {
    perror("tcp bind error: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }
  if (bind(socket_udp_, (struct sockaddr *)(&address_), sizeof(address_)) ==
      -1)
  {
    perror("udp bind error: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }

  int flags_tcp = fcntl(socket_tcp_, F_GETFL, 0); // получаем флаги
  if (flags_tcp == -1)
  {
    perror("fcntl tcp get error: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }
  if (fcntl(socket_tcp_, F_SETFL, flags_tcp | O_NONBLOCK) ==
      -1) // дописываем флаги
  {
    perror("fcntl tcp set error: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }

  int flags_udp = fcntl(socket_udp_, F_GETFL, 0); // получаем флаги
  if (flags_udp == -1)
  {
    perror("fcntl udp get error: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }
  if (fcntl(socket_udp_, F_SETFL, flags_udp | O_NONBLOCK) ==
      -1) // дописываем флаги
  {
    perror("fcntl udp set error: ");
    close(socket_tcp_);
    close(socket_udp_);
    return false;
  }

  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ == -1)
  {
    close(socket_tcp_);
    close(socket_udp_);
    perror("epoll_create1 error: ");
    return false;
  }

  struct epoll_event ev_tcp;
  ev_tcp.events = EPOLLIN;
  ev_tcp.data.fd = socket_tcp_;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_tcp_, &ev_tcp) == -1)
  {
    close(socket_tcp_);
    close(socket_udp_);
    close(epoll_fd_);
    return false;
  }

  struct epoll_event ev_udp;
  ev_udp.events = EPOLLIN;
  ev_udp.data.fd = socket_udp_;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_udp_, &ev_udp) == -1)
  {
    close(socket_tcp_);
    close(socket_udp_);
    close(epoll_fd_);
    return false;
  }
  wake_up_fd = eventfd(0, EFD_NONBLOCK);
  if (wake_up_fd == -1)
  {
    close(socket_tcp_);
    close(socket_udp_);
    close(epoll_fd_);
    perror("eventfd error: ");
    return false;
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = wake_up_fd;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wake_up_fd, &ev) == -1)
  {
    perror("epoll_ctl add wake_up_fd error: ");
    close(wake_up_fd);
    close(socket_tcp_);
    close(socket_udp_);
    close(epoll_fd_);
    return false;
  }

  running_ = true;
  if (listen(socket_tcp_, MAX_EVENTS_ == -1))
  {
    close(wake_up_fd);
    close(socket_tcp_);
    close(socket_udp_);
    close(epoll_fd_);
    perror("listen tcp error");
    return false;
  }
  std::cout << "Server is running port: " << port << std::endl;
  return true;
}

Server::~Server()
{
  std::cout << "Server destructor\n";
  if (shutdown_ == false)
  {
    stop();
  }
}

void Server::run()
{
  epoll_event events[MAX_EVENTS_];
  while (running_)
  {
    int fd_ready = epoll_wait(epoll_fd_, events, MAX_EVENTS_, -1);
    if (fd_ready == -1)
    {
      if (errno == EINTR)
      {
        continue;
      }
      else
      {
        perror("epoll_wait error: ");
        break;
      }
    }
    for (int i = 0; i < fd_ready; i++)
    {
      if (events[i].data.fd == socket_tcp_)
      {
        handleTcpConnection();
      }
      else if (events[i].data.fd == socket_udp_)
      {
        handleUdpMessage();
      }
      else if (events[i].data.fd == wake_up_fd)
      {
        return;
      }
      else
      {
        handleTcpMessage(events[i].data.fd);
      }
    }
  }
}

void Server::handleTcpConnection()
{

  sockaddr_in client_addr{};
  socklen_t addr_len = sizeof(client_addr);

  int client_fd = accept(socket_tcp_, (sockaddr *)&client_addr, &addr_len);
  if (client_fd == -1)
  {
    perror("accept");
    return;
  }

  epoll_event event{};
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = client_fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &event) == -1)
  {
    perror("epoll_ctl client");
    close(client_fd);
    return;
  }

  ClientInfo client_info{client_fd, client_addr};
  {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[client_fd] = client_info;
  }

  total_clients_++;
  current_clients_++;

  std::cout << "New TCP connection from " << inet_ntoa(client_addr.sin_addr)
            << ":" << ntohs(client_addr.sin_port)
            << " (total: " << total_clients_
            << ", current: " << current_clients_ << ")" << std::endl;
}

void Server::handleUdpMessage()
{

  char buffer[BUFFER_SIZE_];
  sockaddr_in client_addr{};
  socklen_t addr_len = sizeof(client_addr);

  ssize_t bytes_received = recvfrom(socket_udp_, buffer, sizeof(buffer) - 1, 0,
                                    (sockaddr *)&client_addr, &addr_len);

  if (bytes_received <= 0)
  {

    return;
  }
  buffer[bytes_received] = '\0';
  std::string message(buffer);
  if (!message.empty() && message.back() == '\n')
  {
    message.pop_back();
  }
  if (!message.empty() && message.back() == '\r')
  {
    message.pop_back();
  }

  thread_pool->enqueue(
      [this, addr_len, client_addr, message = std::move(message)]()
      {
        processUdpMessage(client_addr, addr_len, message);
      });
}

void Server::handleTcpMessage(int client_fd)
{

  char buffer[BUFFER_SIZE_];
  ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received <= 0)
  {
    closeClient(client_fd);
    return;
  }
  buffer[bytes_received] = '\0';
  std::string message(buffer);
  if (!message.empty() && message.back() == '\n')
  {
    message.pop_back();
  }
  if (!message.empty() && message.back() == '\r')
  {
    message.pop_back();
  }
  sockaddr_in client_addr{};
  {
    std::lock_guard lock(clients_mutex_);
    auto iter = clients_.find(client_fd);
    if (iter != clients_.end())
    {
      client_addr = iter->second.client_addr_;
    }
    else
    {
      return;
    }
  }
  thread_pool->enqueue(
      [this, client_fd, client_addr, message = std::move(message)]()
      {
        processTcpMessage(client_fd, client_addr, message);
      });
}

void Server::stop()
{

  running_ = false;
  {
    std::lock_guard lock(clients_mutex_);
    for (auto &client : clients_)
    {
      close(client.first);
    }
    clients_.clear();
  }
  uint64_t one = 1;
  (void)write(wake_up_fd, &one, sizeof(one));
  if (socket_tcp_ != -1)
    close(socket_tcp_);
  if (socket_udp_ != -1)
    close(socket_udp_);
  if (epoll_fd_ != -1)
    close(epoll_fd_);
  if (wake_up_fd != -1)
    close(wake_up_fd);
  std::cout << "Server stopped\n";
}

void Server::processTcpMessage(int client_fd, sockaddr_in client_addr,
                               const std::string &message)
{

  auto response = processMessage(message);
  (void)send(client_fd, response.c_str(), response.size(), 0);
}

void Server::processUdpMessage(sockaddr_in client_addr, socklen_t len,
                               const std::string &message)
{
  auto response = processMessage(message);
  (void)sendto(socket_udp_, response.c_str(), response.size(), 0,
               (sockaddr *)&client_addr, len);
}

std::string Server::processMessage(const std::string &message)
{
  if (message.empty())
  {
    return "";
  }
  if (message[0] == '/')
  {
    if (message == "/time")
    {
      return getCurrentTime();
    }
    else if (message == "/stats")
    {
      return getStats();
    }
    else if (message == "/shutdown")
    {
      std::cout << "Shutdown command received\n";
      std::cout << "Server shutting down\n";
      shutdown_ = true;
      stop();
    }
    else
    {
      std::cout << "Unknown command: " << message << std::endl;
    }
  }
  else
  {
    return message;
  }
  return "";
}

std::string Server::getCurrentTime()
{
  auto now = time(nullptr);
  tm time_info;
  (void)localtime_r(&now, &time_info);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time_info);
  return std::string(buffer);
}

std::string Server::getStats()
{

  return "Total clients: " + std::to_string(total_clients_) +
         " current clients: " + std::to_string(current_clients_);
}

void Server::closeClient(int client)
{
  std::lock_guard lock(clients_mutex_);
  auto iter = clients_.find(client);

  if (iter != clients_.end())
  {
    std::cout << "Remove client\n";
    clients_.erase(client);
    current_clients_--;
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client, nullptr);
    close(client);
  }
}
