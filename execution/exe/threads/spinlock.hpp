#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/spin.hpp>

namespace exe::threads {

// Test-and-TAS spinlock

class SpinLock {
 public:
  void Lock() {
    twist::ed::SpinWait spin_wait;
    while (locked_.exchange(true, std::memory_order::acquire)) {
      while (locked_.load(std::memory_order::relaxed)) {
        spin_wait();
      }
    }
  }

  bool TryLock() {
    bool free = false;
    return locked_.compare_exchange_strong(
        free, true, std::memory_order::acquire, std::memory_order::relaxed);
  }

  void Unlock() {
    locked_.store(false, std::memory_order::release);
  }

  // Lockable

  void lock() {  // NOLINT
    Lock();
  }

  bool try_lock() {  // NOLINT
    return TryLock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  twist::ed::stdlike::atomic<bool> locked_{false};
};

}  // namespace exe::threads
