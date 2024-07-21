#include <exe/executors/tp/compute/thread_pool.hpp>

#include <twist/ed/local/ptr.hpp>

#include <wheels/core/panic.hpp>

namespace exe::executors::tp::compute {
static twist::ed::ThreadLocalPtr<ThreadPool> pool = nullptr;

ThreadPool::ThreadPool(size_t threads)
    : num_threads_(threads) {
}

void ThreadPool::Start() {
  for (size_t i = 0; i < num_threads_; ++i) {
    workers_.emplace_back([this] {
      Worker();
    });
  }
}

void ThreadPool::Worker() {
  pool = this;

  while (true) {
    auto task = tasks_.Take();
    if (task == std::nullopt) {
      return;
    }
    (*task)->Run();
    tasks_wg_.Done();
  }
}

ThreadPool::~ThreadPool() {
  assert(workers_.empty());
}

void ThreadPool::Submit(TaskBase* task) {
  tasks_wg_.Add(1);
  tasks_.Put(task);
}

ThreadPool* ThreadPool::Current() {
  return pool;
}

void ThreadPool::WaitIdle() {
  tasks_wg_.Wait();
}

void ThreadPool::Stop() {
  tasks_.Close();
  for (size_t i = 0; i < num_threads_; ++i) {
    workers_[i].join();
  }
  workers_.clear();
}

}  // namespace exe::executors::tp::compute
