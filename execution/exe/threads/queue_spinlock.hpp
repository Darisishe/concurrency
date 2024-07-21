#pragma once

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/spin.hpp>

/*
 * Scalable Queue SpinLock
 *
 * Usage:
 *
 * QueueSpinLock spinlock;
 *
 * {
 *   QueueSpinLock::Guard guard(spinlock);  // <-- Acquire
 *   // <-- Critical section
 * }  // <-- Release
 *
 */

class QueueSpinLock {
 public:
  class Guard {
    friend class QueueSpinLock;

   public:
    explicit Guard(QueueSpinLock& host)
        : host_(host) {
      host_.Acquire(this);
    }

    ~Guard() {
      host_.Release(this);
    }

   private:
    QueueSpinLock& host_;
    twist::ed::stdlike::atomic<bool> is_owner_ = false;
    twist::ed::stdlike::atomic<Guard*> next_ = nullptr;
  };

 private:
  // CS-Release (Acquire) used to introduce Happens-Before between Critical Sections
  // everything else (B-Release/Acquire, etc.) is due to non-atomicity of std::atomic constructor
  void Acquire(Guard* waiter) {
    Guard* prev_tail = tail_.exchange(waiter, std::memory_order_acq_rel); // CS-Acquire-2
    if (prev_tail == nullptr) {
      return;
    }

    // Atomic`s constructors is not performed atomically (https://en.cppreference.com/w/cpp/atomic/atomic/atomic)
    // so we need Releasing (prev_tail's) thread to see our fields by Release-Acquire pair
    // Remark: we see prev_tail's fields due to tail_.exchange(...) above, so it's valid to access prev_tail->next_
    // Because of that we only need to make RELEASING THREAD see ACQUIRING THREAD guard atomics
    prev_tail->next_.store(waiter, std::memory_order_release); // B-Release

    twist::ed::SpinWait spin_wait;
    while (!waiter->is_owner_.load(std::memory_order_acquire)) { // CS-Acquire-1
      spin_wait();
    }
  }

  void Release(Guard* owner) {
    Guard* next = owner->next_.load(std::memory_order_acquire); // B-Acquire
    if (next != nullptr) {
      next->is_owner_.store(true, std::memory_order_release); // CS-Release-1
      return;
    }

    Guard* tmp = owner;
    if (!tail_.compare_exchange_strong(tmp, nullptr, std::memory_order_release,  // CS-Release-2
                                       std::memory_order_relaxed)) {
      twist::ed::SpinWait spin_wait;
      while ((next = owner->next_.load(std::memory_order_acquire)) == nullptr) {  // B-Acquire (to see is_owner_ field)
        spin_wait();
      }

      next->is_owner_.store(true, std::memory_order_release); // CS-Release-1 (to introduce Happens-Before between Critical Sections)
    }
  }

 private:
  twist::ed::stdlike::atomic<Guard*> tail_ = nullptr;
};
