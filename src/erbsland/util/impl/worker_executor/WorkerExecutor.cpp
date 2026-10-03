#include "WorkerExecutor.hpp"

#include "WorkerAdmissionError.hpp"
#include "WorkerExecutorState.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <mutex>
#include <thread>
#include <utility>

namespace erbsland::util::impl {

using namespace text::literals;

WorkerExecutor::WorkerExecutor(
    const unsigned int maximumWorkers,
    const std::size_t maximumOperations,
    const unsigned int maximumSecurityWorkers,
    const std::chrono::milliseconds idleTimeout) :
    _state{std::make_shared<WorkerExecutorState>()} {
    if (maximumWorkers == 0U || maximumOperations == 0U || maximumSecurityWorkers == 0U ||
        maximumSecurityWorkers > maximumWorkers || idleTimeout.count() <= 0) {
        throw err::ParameterFailure{"Invalid background execution limits."_el, "limits"_el};
    }
    _state->maximumWorkers = maximumWorkers;
    _state->maximumOperations = maximumOperations;
    _state->concurrency = {maximumWorkers, maximumSecurityWorkers};
    _state->minimumWorkers = std::min(maximumWorkers, std::max(4U, std::thread::hardware_concurrency()));
    _state->idleTimeout = idleTimeout;
}

WorkerExecutor::~WorkerExecutor() {
    {
        const auto lock = std::scoped_lock{_state->mutex};
        _state->stopping = true;
    }
    _state->condition.notify_all();
}

auto WorkerExecutor::reserve(const WorkClass workClass) -> WorkerReservation {
    if (workClass != WorkClass::General && workClass != WorkClass::Security) {
        throw err::ParameterFailure{"Invalid background work class."_el, "workClass"_el};
    }
    auto operation = std::make_shared<WorkerOperation>();
    {
        const auto lock = std::scoped_lock{_state->mutex};
        const auto classIndex = static_cast<std::size_t>(workClass);
        const auto securityLimit = std::max(std::size_t{1U}, _state->maximumOperations / 2U);
        if (_state->stopping || _state->operationCount >= _state->maximumOperations ||
            (workClass == WorkClass::Security && _state->admitted[classIndex] >= securityLimit)) {
            throw WorkerAdmissionError{"The background operation admission limit has been reached."_el};
        }
        ++_state->operationCount;
        ++_state->admitted[classIndex];
        operation->state = _state;
        operation->workClass = classIndex;
    }
    return WorkerReservation{std::move(operation)};
}

void WorkerExecutor::submit(std::function<void()> work, const WorkClass workClass) {
    reserve(workClass).submit(std::move(work));
}

}
