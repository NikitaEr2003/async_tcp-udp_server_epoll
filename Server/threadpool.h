#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class Threadpool
{
public:
  explicit Threadpool(size_t thread_count); // Принимает количество потоков
  ~Threadpool();

private:
  std::vector<std::thread> workers_;        // Тут хранятся потоки
  std::queue<std::function<void()>> tasks_; // Очередь задач
  std::mutex queue_mutex;
  std::condition_variable cv_;
  std::atomic<bool> stop_{false}; // Флаг для остановки

public:
  template <typename F> // Функция которая кладёт в работу задачи
  void enqueue(F &&f)
  {
    if (stop_)
    {
      return;
    }
    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      tasks_.emplace(std::forward<F>(f));
    }
    cv_.notify_one();
  }
};