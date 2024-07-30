#include <exe/executors/strand.hpp>

namespace exe::executors {

Strand::Strand(IExecutor& underlying)
    : handler_(new strand::QueueHandler(underlying)) {
}

void Strand::Submit(TaskBase* cs) {
  handler_->Submit(cs);
}

Strand::~Strand() {
  handler_->Discard();
}

}  // namespace exe::executors