#pragma once

#include <exe/executors/executor.hpp>

#include <exe/support/mpmc_queue.hpp>
#include <exe/support/wait_group.hpp>

#include <twist/ed/stdlike/thread.hpp>

#include <vector>

namespace exe::executors::tp::compute {

// Thread pool for independent CPU-bound tasks
// Fixed pool of worker threads + shared unbounded blocking queue

class ThreadPool : public IExecutor {
 public:
  explicit ThreadPool(size_t threads);
  ~ThreadPool();

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Non-movable
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  void Start();

  // IExecutor
  void Submit(TaskBase*) override;

  static ThreadPool* Current();

  void WaitIdle();

  void Stop();

 private:
  void Worker();

 private:
  // Worker threads, task queue, etc
  std::vector<twist::ed::stdlike::thread> workers_;
  size_t num_threads_;
  support::UnboundedBlockingQueue<TaskBase> tasks_;
  support::WaitGroup tasks_wg_;  // counts tasks
};

}  // namespace exe::executors::tp::compute
