#include <exception>
#include <exe/fibers/core/fiber.hpp>

#include <twist/ed/local/ptr.hpp>

namespace exe::fibers {
static twist::ed::ThreadLocalPtr<Fiber> active_fiber = nullptr;

Fiber::Fiber(Routine routine, Scheduler* sched)
    : coro_(std::move(routine),
            [] {
              std::terminate();
            }),
      scheduler_(sched) {
}

void Fiber::Schedule() {
  scheduler_->Submit(this);
}

void Fiber::Run() noexcept {
  Fiber* caller_fiber = active_fiber;
  active_fiber = this;
  coro_.Resume();
  // after switching back:
  active_fiber = caller_fiber;

  if (!coro_.IsCompleted()) {
    Schedule();
  } else {
    delete this;
  }
}

Fiber* Fiber::Self() {
  return active_fiber;
}

void Fiber::SuspendCoroutine() {
  coro_.Suspend();
}

}  // namespace exe::fibers