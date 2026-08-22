// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartManager.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../event/Events.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::core::impl {

using namespace text::literals;

void ApplicationPartManager::start() {
    {
        const auto lock = std::scoped_lock{_mutex};
        if (!_prepared) {
            throw err::LogicError{"The application-part manager must be prepared before startup."_el};
        }
        if (_state == ApplicationPartManagerState::Starting || _state == ApplicationPartManagerState::Running) {
            return;
        }
        if (_state != ApplicationPartManagerState::Ready) {
            throw err::LogicError{"The application-part manager lifecycle is one-shot."_el};
        }
    }
    invokeControl([weak = weak_from_this()]() -> void {
        if (const auto self = weak.lock(); self != nullptr) {
            self->handleStartAll();
        }
    });
}

void ApplicationPartManager::start(const ApplicationPartIdentifierPtr &identifier) {
    const auto number = resolveIdentifier(identifier);
    {
        const auto lock = std::scoped_lock{_mutex};
        if (!_prepared) {
            throw err::LogicError{"The application-part manager must be prepared before startup."_el};
        }
        if (_state == ApplicationPartManagerState::Stopping || _state == ApplicationPartManagerState::Stopped ||
            _state == ApplicationPartManagerState::Failed) {
            throw err::LogicError{"The application-part manager lifecycle is one-shot."_el};
        }
        auto marks = std::vector<uint8_t>(_records.size(), 0);
        validateDependencyClosureCanStartLocked(number, marks);
    }
    invokeControl([weak = weak_from_this(), number]() -> void {
        if (const auto self = weak.lock(); self != nullptr) {
            self->handleStartPart(number);
        }
    });
}

void ApplicationPartManager::stop(const ApplicationPartIdentifierPtr &identifier) {
    const auto number = resolveIdentifier(identifier);
    invokeControl([weak = weak_from_this(), number]() -> void {
        if (const auto self = weak.lock(); self != nullptr) {
            self->handleStopPart(number);
        }
    });
}

void ApplicationPartManager::stop() {
    {
        const auto lock = std::scoped_lock{_mutex};
        if (!_prepared) {
            throw err::LogicError{"The application-part manager must be prepared before shutdown."_el};
        }
        if (_state == ApplicationPartManagerState::Stopping || _state == ApplicationPartManagerState::Stopped ||
            _state == ApplicationPartManagerState::Failed) {
            return;
        }
    }
    invokeControl([weak = weak_from_this()]() -> void {
        if (const auto self = weak.lock(); self != nullptr) {
            self->handleStopAll();
        }
    });
}

void ApplicationPartManager::handleStartAll() {
    const auto currentState = state();
    if (currentState != ApplicationPartManagerState::Ready) {
        return;
    }
    setManagerState(ApplicationPartManagerState::Starting);
    progressStartup();
}

void ApplicationPartManager::handleStartPart(const std::size_t number) {
    if (state() == ApplicationPartManagerState::Ready) {
        setManagerState(ApplicationPartManagerState::Starting);
    }
    requestDependencyClosure(number);
    progressStartup();
}

void ApplicationPartManager::validateDependencyClosureCanStartLocked(
    const std::size_t number, std::vector<uint8_t> &marks) const {
    auto &mark = marks[number - 1];
    if (mark != 0) {
        return;
    }
    mark = 1;
    const auto &record = _records[number - 1];
    if (record.state == ApplicationPartState::Stopped || record.state == ApplicationPartState::Failed) {
        throw err::LogicError{"A stopped application part can not be restarted."_el};
    }
    for (const auto dependency : record.dependencies) {
        validateDependencyClosureCanStartLocked(dependency, marks);
    }
}

void ApplicationPartManager::requestDependencyClosure(const std::size_t number) {
    auto dependencies = std::vector<std::size_t>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        if (record.state == ApplicationPartState::Stopped || record.state == ApplicationPartState::Failed) {
            return;
        }
        record.manualRequested = true;
        dependencies = record.dependencies;
    }
    for (const auto dependency : dependencies) {
        requestDependencyClosure(dependency);
    }
}

void ApplicationPartManager::progressStartup() {
    auto madeProgress = true;
    while (madeProgress) {
        madeProgress = false;
        for (std::size_t number = 1; number <= _records.size(); ++number) {
            auto part = ApplicationPartPtr{};
            auto evaluateAutomatic = false;
            auto shouldStart = false;
            {
                const auto lock = std::scoped_lock{_mutex};
                auto &record = _records[number - 1];
                if (record.state != ApplicationPartState::Uninitialized || record.stopRequested ||
                    !dependenciesRunning(record)) {
                    continue;
                }
                if (record.manualRequested) {
                    shouldStart = true;
                } else if (_state == ApplicationPartManagerState::Starting && !record.automaticEvaluated) {
                    record.automaticEvaluated = true;
                    evaluateAutomatic = true;
                    part = record.part;
                }
            }
            if (evaluateAutomatic) {
                try {
                    shouldStart = part->automaticStart();
                } catch (...) {
                    handlePartFailure(number, std::current_exception());
                    madeProgress = true;
                    continue;
                }
            }
            if (shouldStart) {
                startPart(number);
                madeProgress = true;
            }
        }
    }
    if (state() == ApplicationPartManagerState::Starting) {
        auto hasStarting = false;
        {
            const auto lock = std::scoped_lock{_mutex};
            for (const auto &record : _records) {
                if (record.state == ApplicationPartState::Starting) {
                    hasStarting = true;
                    break;
                }
            }
        }
        if (!hasStarting) {
            setManagerState(ApplicationPartManagerState::Running);
        }
    }
}

auto ApplicationPartManager::dependenciesRunning(const ApplicationPartRecord &record) const noexcept -> bool {
    for (const auto dependency : record.dependencies) {
        if (_records[dependency - 1].state != ApplicationPartState::Running) {
            return false;
        }
    }
    return true;
}

void ApplicationPartManager::startPart(const std::size_t number) {
    auto part = ApplicationPartPtr{};
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        if (record.state != ApplicationPartState::Uninitialized) {
            return;
        }
        part = record.part;
    }
    const auto weak = weak_from_this();
    auto thread = std::make_shared<ApplicationPartThread>(
        [weak, part, number]() -> void {
            part->initialize();
            part->running();
            if (const auto self = weak.lock(); self != nullptr) {
                self->invokeControl([weak, number]() -> void {
                    if (const auto manager = weak.lock(); manager != nullptr) {
                        manager->handlePartRunning(number);
                    }
                });
            }
        },
        [weak, part, number]() -> void {
            part->cleanup();
            if (const auto self = weak.lock(); self != nullptr) {
                self->invokeControl([weak, number]() -> void {
                    if (const auto manager = weak.lock(); manager != nullptr) {
                        manager->handlePartExited(number);
                    }
                });
            }
        },
        [weak, number](std::exception_ptr error) -> void {
            if (const auto self = weak.lock(); self != nullptr) {
                self->invokeControl([weak, number, error = std::move(error)]() mutable -> void {
                    if (const auto manager = weak.lock(); manager != nullptr) {
                        manager->handlePartFailure(number, std::move(error));
                    }
                });
            }
        });
    part->bind(ApplicationPartManagerAccessWeakPtr{shared_from_this()}, thread->events());
    {
        const auto lock = std::scoped_lock{part->_stoppingMutex};
        part->_completeStoppingFn = [weakThread = std::weak_ptr<ApplicationPartThread>{thread}]() -> void {
            if (const auto lockedThread = weakThread.lock(); lockedThread != nullptr) {
                lockedThread->quit();
            }
        };
    }
    {
        const auto lock = std::scoped_lock{_mutex};
        _records[number - 1].thread = thread;
    }
    setPartState(number, ApplicationPartState::Starting);
    try {
        thread->start();
    } catch (...) {
        handlePartFailure(number, std::current_exception());
        handlePartExited(number);
    }
}

void ApplicationPartManager::handlePartRunning(const std::size_t number) {
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_records[number - 1].failed) {
            return;
        }
    }
    setPartState(number, ApplicationPartState::Running);
    progressStartup();
    progressStopping();
}

void ApplicationPartManager::handleStopAll() {
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_state == ApplicationPartManagerState::Stopped || _state == ApplicationPartManagerState::Failed) {
            return;
        }
        _stopAllRequested = true;
        for (auto &record : _records) {
            record.stopRequested = true;
        }
    }
    setManagerState(ApplicationPartManagerState::Stopping);
    progressStopping();
}

void ApplicationPartManager::handleStopPart(const std::size_t number) {
    requestDependentClosure(number);
    progressStopping();
}

void ApplicationPartManager::requestDependentClosure(const std::size_t number) {
    auto dependents = std::vector<std::size_t>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        record.stopRequested = true;
        dependents = record.dependents;
    }
    for (const auto dependent : dependents) {
        requestDependentClosure(dependent);
    }
}

void ApplicationPartManager::progressStopping() {
    auto madeProgress = true;
    while (madeProgress) {
        madeProgress = false;
        for (std::size_t number = 1; number <= _records.size(); ++number) {
            auto state = ApplicationPartState::Uninitialized;
            auto stopRequested = false;
            auto ready = false;
            {
                const auto lock = std::scoped_lock{_mutex};
                const auto &record = _records[number - 1];
                state = record.state;
                stopRequested = record.stopRequested;
                ready = dependentsTerminal(record);
            }
            if (!stopRequested || !ready) {
                continue;
            }
            if (state == ApplicationPartState::Uninitialized) {
                setPartState(number, ApplicationPartState::Stopped);
                madeProgress = true;
            } else if (state == ApplicationPartState::Running) {
                stopPart(number);
                madeProgress = true;
            }
        }
    }
    finishManagerIfStopped();
}

auto ApplicationPartManager::dependentsTerminal(const ApplicationPartRecord &record) const noexcept -> bool {
    for (const auto dependent : record.dependents) {
        const auto state = _records[dependent - 1].state;
        if (state != ApplicationPartState::Stopped && state != ApplicationPartState::Failed) {
            return false;
        }
    }
    return true;
}

void ApplicationPartManager::stopPart(const std::size_t number) {
    auto part = ApplicationPartPtr{};
    auto thread = ApplicationPartThreadPtr{};
    auto generation = uint64_t{};
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        if (record.state != ApplicationPartState::Running) {
            return;
        }
        part = record.part;
        thread = record.thread;
        generation = ++record.timeoutGeneration;
    }
    setPartState(number, ApplicationPartState::Stopping);
    try {
        thread->requestStop([part]() -> void { part->stopping(); });
        const auto timeout = part->shutdownTimeout();
        if (timeout.isPositive()) {
            _controlEvents->invokeAfter(timeout, [weak = weak_from_this(), number, generation]() -> void {
                if (const auto self = weak.lock(); self != nullptr) {
                    self->recordControlThread();
                    self->handlePartTimeout(number, generation);
                }
            });
        }
    } catch (...) {
        handlePartFailure(number, std::current_exception());
        thread->quit();
    }
}

void ApplicationPartManager::handlePartExited(const std::size_t number) {
    auto thread = ApplicationPartThreadPtr{};
    auto failed = false;
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        thread = record.thread;
        failed = record.failed;
        ++record.timeoutGeneration;
    }
    if (thread != nullptr) {
        thread->join();
    }
    setPartState(number, failed ? ApplicationPartState::Failed : ApplicationPartState::Stopped);
    progressStartup();
    progressStopping();
}

void ApplicationPartManager::handlePartTimeout(const std::size_t number, const uint64_t generation) {
    auto thread = ApplicationPartThreadPtr{};
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        if (record.state != ApplicationPartState::Stopping || record.timeoutGeneration != generation) {
            return;
        }
        thread = record.thread;
    }
    handlePartFailure(
        number, std::make_exception_ptr(err::RuntimeError{"The application part exceeded its shutdown timeout."_el}));
    if (thread != nullptr) {
        thread->quit();
    }
}

void ApplicationPartManager::handlePartFailure(const std::size_t number, std::exception_ptr error) {
    addError(error);
    auto handler = ApplicationPartErrorHandler{};
    auto identifier = ApplicationPartIdentifierPtr{};
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        record.failed = true;
        record.stopRequested = true;
        identifier = record.identifier;
        handler = _errorHandler;
    }
    requestDependentClosure(number);
    auto action = ApplicationPartErrorAction::StopAll;
    if (handler) {
        try {
            action = handler(identifier, error);
        } catch (...) {
            addError(std::current_exception());
            action = ApplicationPartErrorAction::StopAll;
        }
    }
    if (action == ApplicationPartErrorAction::StopAll) {
        {
            const auto lock = std::scoped_lock{_mutex};
            _fatalFailure = true;
        }
        handleStopAll();
    } else {
        progressStopping();
    }
}

void ApplicationPartManager::finishManagerIfStopped() {
    auto finished = false;
    auto failed = false;
    {
        const auto lock = std::scoped_lock{_mutex};
        if (!_stopAllRequested) {
            return;
        }
        finished = true;
        for (const auto &record : _records) {
            if (record.state != ApplicationPartState::Stopped && record.state != ApplicationPartState::Failed) {
                finished = false;
                break;
            }
        }
        failed = _fatalFailure;
    }
    if (finished) {
        setManagerState(failed ? ApplicationPartManagerState::Failed : ApplicationPartManagerState::Stopped);
    }
}

}
