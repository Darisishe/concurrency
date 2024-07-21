#include <exe/executors/strand.hpp>
#include <exe/executors/submit.hpp>

namespace exe::executors {

Strand::StrandState::StrandState(IExecutor& underlying)
    : underlying_(underlying) {
}

void Strand::StrandState::ExecuteBatch(List batch) {
  while (!batch.IsEmpty()) {
    batch.PopFront()->Run();
  }
}

/*
Cases of ordering in ProcessQueue():

1.
We want (exchange B) in Thread B to be Synchronize-With (fetch_add A) in
Thread A (when Thread A reads 0 with (fetch_add A)) to make Thread A
see the results of ExecuteBatch in Thread B. (Forcing hb-relation between
critical sections)

2.
We want (fetch_add A) in Thread A to be Synchronize-With (exchange B)
in Thread B in following situation:

a) Thread B performs ExecuteBatch()

b) Thread A executes Strand::Submit() 'before' (exchange B), so Thread A will
Put new task in tasks_, but will leave ProcessQueue() right after (fetch_add A).

So we want Thread B to run ProcessQueue() again after (exchange B), in other
words we want Put() in Thread A to happen-before IsEmpty() check in Thread B

(see test 'MemoryOrder' from suite 'Strand')
*/
void Strand::StrandState::ProcessQueue() {
#if defined(__has_feature) && __has_feature(thread_sanitizer)

  // TSan doesn't work with atomic_thread_fence
  if (contenders_count_.fetch_add(1, std::memory_order::acq_rel) > 0) {
    // queue already in process somewhere
    return;
  }

#else

  // (fetch_add A)
  if (contenders_count_.fetch_add(1, std::memory_order::release) > 0) {
    // queue already in process somewhere
    return;
  }

  // case 1. (Queue was Free)
  twist::ed::stdlike::atomic_thread_fence(std::memory_order::acquire);

#endif

  // working with queue:

  // 'state' variable extends lifetime of StrandState object
  exe::executors::Submit(underlying_, [state = shared_from_this()]() {
    state->ExecuteBatch(state->tasks_.TakeAll());

#if defined(__has_feature) && __has_feature(thread_sanitizer)

    state->contenders_count_.exchange(0, std::memory_order::acq_rel);

#else

    // (exchange B)
    if (state->contenders_count_.exchange(0, std::memory_order::release) > 1) {
      // case 2. (Contention occured)
      twist::ed::stdlike::atomic_thread_fence(std::memory_order::acquire);
    }

#endif

    if (!state->tasks_.IsEmpty()) {
      state->ProcessQueue();
    }
  });
}

Strand::Strand(IExecutor& underlying)
    : state_(std::make_shared<StrandState>(underlying)) {
}

void Strand::Submit(TaskBase* task) {
  state_->tasks_.Put(task);
  state_->ProcessQueue();
}

}  // namespace exe::executors
