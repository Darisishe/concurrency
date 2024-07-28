#pragma once

#include <wheels/intrusive/forward_list.hpp>
#include <optional>

#include <exe/threads/spinlock.hpp>
#include <exe/threads/condvar.hpp>

namespace exe::support {

// Unbounded blocking multi-producers/multi-consumers (MPMC) queue
// Based on IntrusiveForwardList
template <typename T>
class UnboundedBlockingQueue {
 public:
  using List = wheels::IntrusiveForwardList<T>;

  bool Put(T* value) {
    std::lock_guard guard(lock_);
    if (closed_) {
      return false;
    }

    buffer_.PushBack(value);
    closed_or_not_empty_.NotifyOne();
    return true;
  }

  std::optional<T*> Take() {
    std::unique_lock lock(lock_);

    while (!closed_ && buffer_.IsEmpty()) {
      closed_or_not_empty_.Wait(lock);
    }

    if (closed_ && buffer_.IsEmpty()) {
      return std::nullopt;
    } else {
      return buffer_.PopFront();
    }
  }

  void Close() {
    std::lock_guard guard(lock_);
    closed_ = true;
    closed_or_not_empty_.NotifyAll();
  }

 private:
  // Buffer
  List buffer_;  // protected by lock_
  bool closed_ = false;
  threads::SpinLock lock_;
  threads::CondVar closed_or_not_empty_;
};

}  // namespace exe::support
