#include <exe/fibers/sched/go.hpp>

#include <exe/fibers/core/fiber.hpp>

namespace exe::fibers {

void Go(Scheduler& scheduler, Routine routine) {
  Fiber* fiber = new Fiber(std::move(routine), &scheduler);
  fiber->Schedule();
}

void Go(Routine routine) {
  Go(*Fiber::Self()->GetScheduler(), std::move(routine));
}

}  // namespace exe::fibers
