#pragma once

#include <sure/context.hpp>
#include <sure/stack.hpp>

#include <function2/function2.hpp>

#include <exception>

namespace exe::coro {

class CoroutineCore : private sure::ITrampoline {
 public:
  using Routine = fu2::unique_function<void()>;
  using ExceptionHandler = fu2::unique_function<void()>;

  explicit CoroutineCore(Routine routine, ExceptionHandler handler);

  void Resume();

  bool IsCompleted() const;

  // Suspends coroutine
  void Suspend();

 private:
  void SetupExecutionContext();

  [[noreturn]] void Run() noexcept override;

 private:
  Routine routine_;
  sure::Stack coro_stack_;
  sure::ExecutionContext coro_context_;
  sure::ExecutionContext caller_context_;
  ExceptionHandler exception_handler_;  // reaction to exception in routine_()
  bool is_completed_ = false;
};

}  // namespace exe::coro
