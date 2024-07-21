#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>

#include <fmt/core.h>

namespace stdlike {

class CondVar {
 public:
  // Mutex - BasicLockable
  // https://en.cppreference.com/w/cpp/named_req/BasicLockable
  template <class Mutex>
  void Wait(Mutex& mutex) {
    auto curr_stage = stage_.load();
    mutex.unlock();
    twist::ed::Wait(stage_, curr_stage, std::memory_order_relaxed);
    mutex.lock();
  }

  void NotifyOne() {
    twist::ed::WakeKey wake_key = twist::ed::PrepareWake(stage_);
    stage_.fetch_add(1, std::memory_order_relaxed);
    twist::ed::WakeOne(wake_key);
  }

  void NotifyAll() {
    twist::ed::WakeKey wake_key = twist::ed::PrepareWake(stage_);
    stage_.fetch_add(1, std::memory_order_relaxed);
    twist::ed::WakeAll(wake_key);
  }

 private:
  twist::ed::stdlike::atomic<uint32_t> stage_{0};
};

}  // namespace stdlike
