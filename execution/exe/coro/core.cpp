#include <exe/coro/core.hpp>

#include <wheels/core/assert.hpp>
#include <wheels/core/compiler.hpp>

namespace exe::coro {

static const size_t kDefaultStackSize = 64 * 1024;

CoroutineCore::CoroutineCore(Routine routine, ExceptionHandler handler)
    : routine_(std::move(routine)),
      coro_stack_(sure::Stack::AllocateBytes(kDefaultStackSize)),
      exception_handler_(std::move(handler)) {
  SetupExecutionContext();
}

void CoroutineCore::SetupExecutionContext() {
  coro_context_.Setup(coro_stack_.MutView(), this);
}

void CoroutineCore::Resume() {
  caller_context_.SwitchTo(coro_context_);
}

void CoroutineCore::Suspend() {
  coro_context_.SwitchTo(caller_context_);
}

void CoroutineCore::Run() noexcept {
  try {
    routine_();
  } catch (...) {
    exception_handler_();
  }
  is_completed_ = true;
  coro_context_.ExitTo(caller_context_);
}

bool CoroutineCore::IsCompleted() const {
  return is_completed_;
}

}  // namespace exe::coro
