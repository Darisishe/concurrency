#include <exe/executors/manual.hpp>

namespace exe::executors {

void ManualExecutor::Submit(TaskBase* task) {
  tasks_.PushBack(task);
}

// Run tasks

size_t ManualExecutor::RunAtMost(size_t limit) {
  size_t count = 0;
  while (count < limit && NonEmpty()) {
    tasks_.PopFront()->Run();  // Running task
    ++count;
  }

  return count;
}

size_t ManualExecutor::Drain() {
  size_t count = 0;
  while (NonEmpty()) {
    count += RunAtMost(TaskCount());
  }
  return count;
}

}  // namespace exe::executors
