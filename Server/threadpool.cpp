#include "threadpool.h"
Threadpool::~Threadpool()
{
  stop_ = true;
  cv_.notify_all();
  for (auto &worker : workers_)
  {
    if (worker.joinable())
      worker.join();
  }
}

Threadpool::Threadpool(size_t thread_count)
{
  workers_.reserve(thread_count);
  for (size_t i = 0; i < thread_count; i++)
  {

    workers_.emplace_back([this]()
                          {
      std::function<void()> task;
      while (true) {
        {
          std::unique_lock<std::mutex> lock(queue_mutex);
          cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });

          if (stop_) {
            return;
          }
          if(tasks_.empty()){
            lock.unlock();
            continue;
          }
          task = tasks_.front();
          tasks_.pop();
        }
        task();
      } });
  }
}