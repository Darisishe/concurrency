#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>

namespace exe::threads {

class CondVar {
 public:
  // Mutex - BasicLockable
  // https://en.cppreference.com/w/cpp/named_req/BasicLockable
  template <class Mutex>
  void Wait(Mutex& mutex) {
    auto curr_stage = stage_.load(std::memory_order_relaxed);
    mutex.unlock();

    parked_.fetch_add(1, std::memory_order_relaxed);
    twist::ed::Wait(stage_, curr_stage, std::memory_order_relaxed);
    parked_.fetch_sub(1, std::memory_order_relaxed);

    mutex.lock();
  }

  void NotifyOne() {
    twist::ed::WakeKey wake_key = twist::ed::PrepareWake(stage_);

    stage_.fetch_add(1, std::memory_order_relaxed);
    if (parked_.load(std::memory_order_relaxed) > 0) {
      twist::ed::WakeOne(wake_key);
    }
  }

  void NotifyAll() {
    twist::ed::WakeKey wake_key = twist::ed::PrepareWake(stage_);

    stage_.fetch_add(1, std::memory_order_relaxed);
    if (parked_.load(std::memory_order_relaxed) > 0) {
      twist::ed::WakeAll(wake_key);
    }
  }

 private:
  twist::ed::stdlike::atomic<uint32_t> stage_{0};

  // we will count parked threads to get rid of unneccessery Wake syscalls
  twist::ed::stdlike::atomic<uint32_t> parked_{0};
};

}  // namespace exe::threads