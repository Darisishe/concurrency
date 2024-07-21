#pragma once

#include "atomic_stamped_ptr.hpp"

#include <twist/ed/stdlike/atomic.hpp>

#include <optional>

// Treiber unbounded lock-free stack
// https://en.wikipedia.org/wiki/Treiber_stack
template <typename T>
class LockFreeStack {
  struct Node {
    T value;
    StampedPtr<Node> next;

    twist::ed::stdlike::atomic<int> inner_counter = 0;
  };

 public:
  void Push(T value) {
    Node* node = new Node{std::move(value), top_.Load(std::memory_order::relaxed)};
    auto new_top = StampedPtr(node, 0);
    while (!top_.CompareExchangeWeak(new_top->next, new_top, std::memory_order::release, std::memory_order::relaxed)) {
    }
  }

  std::optional<T> TryPop() {
    while (true) {
      StampedPtr curr_top = top_.Load(std::memory_order::relaxed);
      if (curr_top.raw_ptr == nullptr) {
        return std::nullopt;
      }

      if (!top_.CompareExchangeWeak(curr_top, curr_top.IncrementStamp(), std::memory_order::acquire, std::memory_order::relaxed)) {
        continue;
      }
      curr_top = curr_top.IncrementStamp();
      auto curr_top_copy = curr_top;
      if (top_.CompareExchangeWeak(curr_top, curr_top->next, std::memory_order::acquire, std::memory_order::relaxed)) {
        T value = std::move(curr_top->value);
        curr_top->inner_counter.fetch_add(curr_top.stamp, std::memory_order::relaxed);

        if (curr_top->inner_counter.fetch_sub(1, std::memory_order::acq_rel) == 1) {
          delete curr_top.raw_ptr;
        }

        return value;

      }
      
      if (curr_top_copy->inner_counter.fetch_sub(1, std::memory_order::acq_rel) == 1) {
        delete curr_top_copy.raw_ptr;
      }
    }
  }

  ~LockFreeStack() {
    while (TryPop()) {
    }
  }

 private:
  AtomicStampedPtr<Node> top_ = AtomicStampedPtr(StampedPtr<Node>{nullptr, 0});
};
