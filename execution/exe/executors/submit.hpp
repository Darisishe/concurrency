#pragma once

#include <exe/executors/executor.hpp>
#include <exe/executors/task.hpp>
#include <type_traits>
#include <utility>

namespace exe::executors {

template <typename F>
class TaskContainer : public TaskBase {
 public:
  // F&& isn't universal referense here (because template parameter is for
  // class, not for ctor) so we need to take two cases into account:

  // 1st case - rvalue argument
  explicit TaskContainer(F&& fun)
      : function_(std::move(fun)) {
  }

  // 2nd case - lvalue argument (copies function object)
  explicit TaskContainer(F& fun)
      : function_(fun) {
  }

  void Run() noexcept override {
    function_();
    delete this;
  }

 private:
  F function_;
};

/*
 * Usage:
 *
 * Submit(thread_pool, [] {
 *   fmt::println("Running on thread pool");
 * });
 *
 */

template <typename F>
void Submit(IExecutor& exe, F&& fun) {
  // accepts lambda-type F, not a fu2::unique_function<void()>

  // creating task-container, that will self-deallocate after execution in
  // executor (using remove_reference to get rid of '&' in case of lvalue)
  using TaskContainerType = TaskContainer<std::remove_reference_t<F>>;
  exe.Submit(new TaskContainerType(std::forward<F>(fun)));
}

}  // namespace exe::executors
