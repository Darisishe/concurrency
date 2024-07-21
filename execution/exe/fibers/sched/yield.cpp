#include <exe/fibers/sched/yield.hpp>
#include <exe/fibers/core/fiber.hpp>

namespace exe::fibers {

void Yield() {
  Fiber::Self()->SuspendCoroutine();
}

}  // namespace exe::fibers