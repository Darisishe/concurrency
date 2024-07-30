#pragma once

#include <exe/support/mpsc_lock_free_queue.hpp>
#include <exe/executors/executor.hpp>

#include "strand_state_machine.hpp"

namespace exe::executors::strand {

// Shared state of Strand
class QueueHandler : TaskBase {
  using List = wheels::IntrusiveForwardList<TaskBase>;
 public:

  explicit QueueHandler(IExecutor& underlying)
      : underlying_(underlying) {
  }

  // called by Strand Destructor
  void Discard() {
    if (state_.Discard()) {
      // Rendezvous occured
      delete this;
    }
  }

  // adds task to queue runs processing
  // (only if queue is not in process already)
  void Submit(TaskBase* task) {
    tasks_.Put(task);

    // performs RELEASE, so other thread will see new task in queue
    state_.Submit();

    if (state_.Process()) {
      // ownership of queue acquired
      SubmitHandler();
    }
  }

  void Run() noexcept override {
    ProcessQueue();
  }

 private:
  static void ExecuteBatch(List batch) {
    while (!batch.IsEmpty()) {
      batch.PopFront()->Run();
    }
  }

  void SubmitHandler() {
    underlying_.Submit(this);
  }

  // creates batch of tasks and submits them
  // used only when ownership acquired (Idle flag is not set)
  void ProcessQueue() {
    // performs ACQUIRE to see new tasks and induce happens-before with previous Critical Sections
    state_.Drain();

    ExecuteBatch(tasks_.TakeAll());

    // if ownership actually released (Idle setted), performs RELEASE to induce HB with next CS
    int state = state_.DismissIfEmpty();

    if (StrandStateMachine::HasEmpty(state) &&
        StrandStateMachine::HasDiscarded(state)) {
      // Rendezvous occured
      delete this;
      return;
    }

    if (StrandStateMachine::HasEmpty(state)) {
      return;
    }

    // there's some tasks to run
    SubmitHandler();
  }

 private:
  IExecutor& underlying_;
  support::MPSCUnboundedLockFreeQueue<TaskBase> tasks_;
  StrandStateMachine state_;
};

}  // namespace exe::executors::strand
