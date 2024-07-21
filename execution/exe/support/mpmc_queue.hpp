#pragma once

#include <wheels/intrusive/forward_list.hpp>
#include <optional>

#include <twist/ed/stdlike/mutex.hpp>
#include <twist/ed/stdlike/condition_variable.hpp>

namespace exe::support {

// Unbounded blocking multi-producers/multi-consumers (MPMC) queue
// Based on IntrusiveForwardList
template <typename T>
class UnboundedBlockingQueue {
 public:
  using List = wheels::IntrusiveForwardList<T>;

  bool Put(T* value) {
    std::lock_guard guard(mutex_);
    if (closed_) {
      return false;
    }

    buffer_.PushBack(value);
    closed_or_not_empty_.notify_one();
    return true;
  }

  std::optional<T*> Take() {
    std::unique_lock lock(mutex_);

    while (!closed_ && buffer_.IsEmpty()) {
      closed_or_not_empty_.wait(lock);
    }

    if (closed_ && buffer_.IsEmpty()) {
      return std::nullopt;
    } else {
      return buffer_.PopFront();
    }
  }

  void Close() {
    std::lock_guard guard(mutex_);
    closed_ = true;
    closed_or_not_empty_.notify_all();
  }

 private:
  // Buffer
  List buffer_;  // protected by mutex_
  bool closed_ = false;
  twist::ed::stdlike::mutex mutex_;
  twist::ed::stdlike::condition_variable closed_or_not_empty_;
};

}  // namespace exe::support
