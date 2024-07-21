#pragma once

#include <exe/fibers/core/routine.hpp>
#include <exe/fibers/core/scheduler.hpp>

namespace exe::fibers {

void Go(Scheduler&, Routine);

void Go(Routine);

}  // namespace exe::fibers