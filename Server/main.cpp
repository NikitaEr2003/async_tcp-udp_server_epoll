#include "server.h"
#include <csignal>
#include <exception>
#include <iostream>

std::atomic<bool> running{true};

void signal_handler(int signum)
{
  std::cout << "\nSignal accepted " << signum << ". closing work..." << std::endl;
  running.store(false);
}

int main(int argc, char *argv[])
{

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  int port = 8888;
  size_t count_threads = std::thread::hardware_concurrency();

  if (argc == 3)
  {
    port = static_cast<int>(std::atoi(argv[1]));
    count_threads = std::atoi(argv[2]);
  }

  try
  {
    Server server(running);
    if (!server.start(port, count_threads))
    {
      return -1;
    }

    server.run();
  }
  catch (std::exception &error)
  {
    std::cout << error.what() << std::endl;
  }
  return 0;
}