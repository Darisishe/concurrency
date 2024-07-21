#pragma once

#include <function2/function2.hpp>
#include <wheels/intrusive/forward_list.hpp>

namespace exe::executors {

struct ITask {
  virtual ~ITask() = default;

  virtual void Run() noexcept = 0;
};

struct TaskBase : ITask,
                  wheels::IntrusiveForwardListNode<TaskBase> {};

}  // namespace exe::executors
