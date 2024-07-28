#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/sys.hpp>

#include <cstdint>

namespace exe::threads {

class Mutex {
  enum State : uint32_t {
    Free = 0,
    Locked = 1,
    LockedWithContention = 2,
  };

 public:
  void Lock() {
    uint32_t free = State::Free;
    if (state_.compare_exchange_strong(free, State::Locked,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
      return;
    }

    while (state_.exchange(State::LockedWithContention,
                           std::memory_order_acquire) != State::Free) {
      twist::ed::Wait(state_, State::LockedWithContention,
                      std::memory_order_relaxed);
    }
  }

  void Unlock() {
    auto wake_key = twist::ed::PrepareWake(state_);
    auto prev_state = state_.exchange(State::Free, std::memory_order_release);
    if (prev_state == State::LockedWithContention) {
      twist::ed::WakeOne(wake_key);
    }
  }

  // BasicLockable
  // https://en.cppreference.com/w/cpp/named_req/BasicLockable

  void lock() {  // NOLINT
    Lock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  twist::ed::stdlike::atomic<uint32_t> state_{State::Free};
};

}  // namespace exe::threads
