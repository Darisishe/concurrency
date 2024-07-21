#pragma once

#include <cstdlib>

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

namespace exe::support {
class WaitGroup {
 public:
  // += count
  void Add(size_t count) {
    std::lock_guard guard(mutex_);
    counter_ += count;
  }

  // =- 1
  void Done() {
    std::lock_guard guard(mutex_);
    --counter_;
    if (counter_ == 0) {
      is_zero_.notify_all();
    }
  }

  // == 0
  // One-shot
  void Wait() {
    std::unique_lock lock(mutex_);
    while (counter_ != 0) {
      is_zero_.wait(lock);
    }
  }

 private:
  size_t counter_ = 0;  // protected by mutex_
  twist::ed::stdlike::mutex mutex_;
  twist::ed::stdlike::condition_variable is_zero_;
};

}  // namespace exe::support