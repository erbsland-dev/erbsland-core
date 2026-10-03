#include "WorkerReservation.hpp"

#include "WorkerExecutorState.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../text/Literals.hpp"

#include <mutex>
#include <utility>

namespace erbsland::util::impl {

using namespace text::literals;

WorkerReservation::WorkerReservation(std::shared_ptr<WorkerOperation> operation) : _operation{std::move(operation)} {
}

auto WorkerReservation::isValid() const noexcept -> bool {
    return _operation != nullptr;
}

void WorkerReservation::submit(std::function<void()> work) const {
    if (_operation == nullptr || !work) {
        throw err::LogicFailure{"Background work requires an admission reservation and a callback."_el};
    }
    const auto state = _operation->state;
    {
        const auto lock = std::scoped_lock{state->mutex};
        if (_operation->queued >= 2U) {
            throw err::LogicFailure{"An operation exceeded its reserved continuation and cleanup capacity."_el};
        }
        if (state->idleWorkers == 0U) {
            state->addWorker();
        }
        state->queue.push_back(WorkerExecutorState::Work{_operation, std::move(work)});
        ++_operation->queued;
    }
    // One submission makes at most one additional operation runnable.
    state->condition.notify_one();
}

}
