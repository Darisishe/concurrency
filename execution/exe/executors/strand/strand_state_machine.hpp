#pragma once

#include <twist/ed/stdlike/atomic.hpp>

namespace exe::executors::strand {

class StrandStateMachine {
 public:
  enum Flag {
    Discarded = 1,  // ~Strand() was called
    Empty = 2,      // there's no pending tasks in queue
    Idle = 4,       // there's no batch running somewhere

    // when all these flags set, shared state should be deleted
    Rendezvous = Discarded | Empty | Idle
  };

  StrandStateMachine()
      : state_(Idle | Empty) {
  }

  // sets Discarded
  bool Discard() {
    int curr_state = state_.fetch_or(Discarded, std::memory_order_relaxed) | Discarded;
    // check whether Randezvous occured
    return RendezvousOccured(curr_state);
  }

  // adds Idle flag if state was only Empty, returns either Randezvous or Empty
  int DismissIfEmpty() {
    int tmp = Flag::Empty;
    state_.compare_exchange_strong(tmp, Empty | Idle, std::memory_order_release, std::memory_order_relaxed);

    // contains either Empty, if state_ was Empty before CAS, or actual value of
    // state_
    return tmp;
  }

  // sets Empty
  void Drain() {
    state_.fetch_or(Empty, std::memory_order_acquire);
  }

  // exclude Idle flag and return whether it was Idle
  bool Process() {
    int old = state_.fetch_and(Discarded | Empty, std::memory_order_relaxed);
    return HasFlags(old, Idle);
  }

  // exclude Empty flag
  void Submit() {
    state_.fetch_and(Discarded | Idle, std::memory_order_release);
  }

  
  static bool HasEmpty(int state) {
    return HasFlags(state, Empty);
  }

  static bool HasDiscarded(int state) {
    return HasFlags(state, Discarded);
  }

 private:
  static bool RendezvousOccured(int state) {
    return HasFlags(state, Rendezvous);
  }

  static bool HasFlags(int state, Flag flags) {
    return (state & flags) == flags;
  }

 private:
  twist::ed::stdlike::atomic<int> state_;
};

}  // namespace exe::executors::strand