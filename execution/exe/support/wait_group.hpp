#pragma once

#include <cstdlib>

#include <twist/ed/stdlike/atomic.hpp>
#include <twist/ed/wait/futex.hpp>

namespace exe::support {
class WaitGroup {
 public:
  void Add(uint32_t count) {
    counter_.fetch_add(count);
  }

  void Done() {
    auto wake_key = twist::ed::futex::PrepareWake(counter_);

    if (counter_.fetch_sub(1) == 1 && parked_.load() > 0) {
      twist::ed::futex::WakeAll(wake_key);
    }
  }

  void Wait() {
    while (true) {
      uint32_t curr_counter = counter_.load();
      if (curr_counter == 0) {
        break;
      }
      
      parked_.fetch_add(1);
      twist::ed::futex::Wait(counter_, curr_counter);
      parked_.fetch_sub(1);
    }
  }

 private:
  twist::ed::stdlike::atomic<uint32_t> counter_{0};
  twist::ed::stdlike::atomic<size_t> parked_{0};
};

}  // namespace exe::support