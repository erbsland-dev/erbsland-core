#include "WorkerOperation.hpp"

#include "WorkerExecutorState.hpp"

#include <mutex>

namespace erbsland::util::impl {

WorkerOperation::~WorkerOperation() {
    if (state == nullptr) {
        return;
    }
    const auto lock = std::scoped_lock{state->mutex};
    --state->operationCount;
    --state->admitted[workClass];
}

}
