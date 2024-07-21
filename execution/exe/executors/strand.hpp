#pragma once

#include <exe/executors/executor.hpp>

#include <exe/support/mpsc_lock_free_queue.hpp>

#include <memory>

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

 private:
  class StrandState : public std::enable_shared_from_this<StrandState> {
    friend class Strand;

   public:
    using List = wheels::IntrusiveForwardList<TaskBase>;

    explicit StrandState(IExecutor& underlying);

    static void ExecuteBatch(List batch);

    // creates batch of tasks and submits them
    // (only if queue is not in process already)
    void ProcessQueue();

   private:
    IExecutor& underlying_;
    support::MPSCUnboundedLockFreeQueue<TaskBase> tasks_;

    // count of 'concurrent' calls to ProcessQueue()
    twist::ed::stdlike::atomic<uint64_t> contenders_count_{0};
  };

 private:
  std::shared_ptr<StrandState> state_;
};

}  // namespace exe::executors
