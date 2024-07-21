#pragma once

#include <exe/fibers/core/routine.hpp>
#include <exe/fibers/core/scheduler.hpp>
#include <exe/executors/task.hpp>

#include <exe/coro/core.hpp>
#include <twist/ed/local/ptr.hpp>

namespace exe::fibers {

// Fiber = stackful coroutine + scheduler (thread-pool/Strand/etc.)

class Fiber : public executors::TaskBase {
 public:
  explicit Fiber(Routine routine, Scheduler* sched);

  void Schedule();

  static Fiber* Self();

  void SuspendCoroutine();

  void Run() noexcept override;

  Scheduler* GetScheduler() {
    return scheduler_;
  }
 private:
  coro::CoroutineCore coro_;
  Scheduler* scheduler_;
};

}  // namespace exe::fibers