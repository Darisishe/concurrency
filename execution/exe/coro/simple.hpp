#pragma once

#include <exe/coro/core.hpp>

namespace exe::coro {

// Simple stackful coroutine
class SimpleCoroutine {
 public:
  using Routine = fu2::unique_function<void()>;

  explicit SimpleCoroutine(Routine routine);

  void Resume();

  // Suspend running coroutine
  static void Suspend();

  bool IsCompleted() const;

 private:
  CoroutineCore coro_;
  std::exception_ptr exception_ = nullptr;
};

}  // namespace exe::coro
