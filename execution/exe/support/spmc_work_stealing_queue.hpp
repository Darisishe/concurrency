#pragma once

#include <array>
#include <span>
#include "twist/ed/stdlike/atomic.hpp"

// Single-Producer / Multi-Consumer Ring Buffer

template <typename Task, size_t Capacity>
class WorkStealingQueue {
  struct Slot {
    twist::ed::stdlike::atomic<Task*> task;

    Slot()
        : task(nullptr) {
    }
  };

 public:
  bool TryPush(Task* task) {
    size_t curr_tail = tail_.load(std::memory_order_relaxed);
    if (curr_tail - head_.load(std::memory_order_relaxed) == Capacity) {
      return false;
    }

    buffer_[curr_tail % Capacity].task.store(task, std::memory_order_relaxed);
    tail_.fetch_add(1, std::memory_order_release);
    return true;
  }

  Task* TryPop() {
    size_t curr_head = head_.fetch_add(1, std::memory_order_relaxed);
      if (curr_head == tail_.load(std::memory_order_relaxed)) {
      tail_.fetch_add(1,std::memory_order_relaxed);
      return nullptr;
    }

    return buffer_[curr_head % Capacity].task.load(std::memory_order_relaxed);
  }

  size_t Grab(std::span<Task*> out_buffer) {
    size_t curr_head = head_.load(std::memory_order_relaxed);
    size_t available_tasks = 0;

    while (true) {
      size_t curr_tail = tail_.load(std::memory_order_acquire);
      if (curr_tail <= curr_head) {
        return 0;
      }
      available_tasks = std::min(curr_tail - curr_head, out_buffer.size());

      for (size_t i = 0; i < available_tasks; ++i) {
        out_buffer[i] = buffer_[(curr_head + i) % Capacity].task.load(std::memory_order_relaxed);
      }

      if (head_.compare_exchange_weak(curr_head, curr_head + available_tasks, std::memory_order_relaxed)) {
        break;
      }
    }

    return available_tasks;
  }

  size_t SpaceLowerBound() const {
    return Capacity - tail_.load(std::memory_order_relaxed) + head_.load(std::memory_order_relaxed);
  }

 private:
  std::array<Slot, Capacity> buffer_;
  twist::ed::stdlike::atomic<size_t> head_ = 0;
  twist::ed::stdlike::atomic<size_t> tail_ = 0;
};
