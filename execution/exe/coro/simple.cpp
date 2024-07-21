#include <exe/coro/simple.hpp>
#include <twist/ed/local/ptr.hpp>

namespace exe::coro {

static twist::ed::ThreadLocalPtr<SimpleCoroutine> active_coroutine = nullptr;

SimpleCoroutine::SimpleCoroutine(Routine routine)
    : coro_(std::move(routine), [this] {
        this->exception_ = std::current_exception();
      }) {
}

void SimpleCoroutine::Resume() {
  SimpleCoroutine* calling_coroutine = active_coroutine;
  active_coroutine = this;
  coro_.Resume();
  // after switch back to caller:
  active_coroutine = calling_coroutine;
  if (exception_ != nullptr) {
    std::rethrow_exception(exception_);
  }
}

void SimpleCoroutine::Suspend() {
  active_coroutine->coro_.Suspend();
}

bool SimpleCoroutine::IsCompleted() const {
  return coro_.IsCompleted();
}

}  // namespace exe::coro