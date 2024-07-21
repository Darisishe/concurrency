#pragma once

#include <wheels/intrusive/forward_list.hpp>
#include <twist/ed/stdlike/atomic.hpp>

namespace exe::support {

// Unbounded lock-free multi-producers/single-consumer (MPSC) stack
// Intrusive
template <typename T>
class MPSCUnboundedLockFreeStack {
  using Node = wheels::IntrusiveForwardListNode<T>;

 public:
  void Push(T* value) {
    Node* node = value;

    node->next_ = top_.load(std::memory_order::relaxed);
    while (!top_.compare_exchange_weak(node->next_, node,
                                       std::memory_order::release,
                                       std::memory_order::relaxed)) {
    }
  }

  template <typename F>
  void ConsumeAll(F&& handler) {
    Node* curr = top_.exchange(nullptr, std::memory_order::acquire);

    while (curr != nullptr) {
      Node* next = curr->next_;
      handler(curr->AsItem());
      curr = next;
    }
  }

  bool IsEmpty() const {
    return top_.load(std::memory_order::relaxed) == nullptr;
  }

 private:
  twist::ed::stdlike::atomic<Node*> top_{nullptr};
};

// Unbounded lock-free multi-producers/single-consumer (MPSC) queue
// Based on MPSCUnboundedLockFreeStack
template <typename T>
class MPSCUnboundedLockFreeQueue {
 public:
  using List = wheels::IntrusiveForwardList<T>;

  void Put(T* value) {
    stack_.Push(value);
  }

  List TakeAll() {
    List reversed;
    stack_.ConsumeAll([&reversed](T* value) {
      reversed.PushFront(value);
    });
    return reversed;
  }

  bool IsEmpty() const {
    return stack_.IsEmpty();
  }

 private:
  MPSCUnboundedLockFreeStack<T> stack_;
};

}  // namespace exe::support