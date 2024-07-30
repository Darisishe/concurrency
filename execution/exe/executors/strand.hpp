#pragma once

#include <exe/executors/strand/queue_handler.hpp>

namespace exe::executors {

// Strand / serial executor / asynchronous mutex

class Strand : public IExecutor {
 public:
  explicit Strand(IExecutor& underlying);

  // Non-copyable
  Strand(const Strand&) = delete;
  Strand& operator=(const Strand&) = delete;

  // Non-movable
  Strand(Strand&&) = delete;
  Strand& operator=(Strand&&) = delete;

  // IExecutor
  void Submit(TaskBase* cs) override;

  ~Strand();

 private:
  strand::QueueHandler* handler_;
};

}  // namespace exe::executors
