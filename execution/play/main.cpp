#include <exe/executors/thread_pool.hpp>
#include <exe/executors/strand.hpp>
#include <exe/executors/submit.hpp>
#include <exe/fibers/sched/go.hpp>
#include <exe/fibers/sched/yield.hpp>

#include <fmt/core.h>

using namespace exe::executors;
using namespace exe;

int main() {
  ThreadPool pool{4};
  Strand strand{pool};

  pool.Start();

  size_t cs = 0;

  for (size_t i = 0; i < 1000; ++i) {
    fibers::Go(strand, [&cs] {
      // Big critical section
      ++cs;
      fibers::Go([&cs] {
        // Small critical section
        ++cs;
      });
      fibers::Yield();
      ++cs;
    });
  }

  pool.WaitIdle();

  fmt::println("# critical sections: {}", cs);

  pool.Stop();
  return 0;
}
