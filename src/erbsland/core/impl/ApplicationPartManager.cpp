// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartManager.hpp"

#include "ApplicationPartIdentifier.hpp"

#include "../../err/LogicError.hpp"
#include "../../event/Events.hpp"
#include "../../event/UnmanagedEventThread.hpp"
#include "../../text/Literals.hpp"

#include <atomic>
#include <thread>
#include <utility>

namespace erbsland::core::impl {

using namespace text::literals;

auto ApplicationPartManager::create(event::EventsPtr controlEvents) -> std::shared_ptr<ApplicationPartManager> {
    auto result = std::shared_ptr<ApplicationPartManager>{
        new ApplicationPartManager{std::move(controlEvents)}, &ApplicationPartManager::destroy};
    result->initializeControlThread();
    return result;
}

ApplicationPartManager::ApplicationPartManager(event::EventsPtr controlEvents) :
    _managerToken{[]() -> uint64_t {
        static auto nextToken = std::atomic<uint64_t>{1};
        return nextToken.fetch_add(1);
    }()},
    _controlEvents{std::move(controlEvents)} {
}

ApplicationPartManager::~ApplicationPartManager() noexcept {
    try {
        for (const auto &record : _records) {
            if (record.thread != nullptr) {
                record.thread->quit();
                record.thread->join();
            }
        }
        if (_ownedControlThread != nullptr) {
            _ownedControlThread->quit();
            _ownedControlThread->join();
        }
    } catch (...) {
        std::terminate();
    }
}

void ApplicationPartManager::initializeControlThread() {
    if (_controlEvents != nullptr) {
        return;
    }
    _ownedControlThread = event::UnmanagedEventThread::create();
    _controlEvents = _ownedControlThread->events();
    _ownedControlThread->start();
}

void ApplicationPartManager::destroy(ApplicationPartManager *manager) noexcept {
    if (manager == nullptr) {
        return;
    }
    if (!manager->isCurrentThreadOwned()) {
        delete manager;
        return;
    }
    try {
        auto cleanupThread = std::thread{[manager]() noexcept -> void { delete manager; }};
        cleanupThread.detach();
    } catch (...) {
        std::terminate();
    }
}

auto ApplicationPartManager::isCurrentThreadOwned() const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    const auto currentThread = std::this_thread::get_id();
    if (_ownedControlThread != nullptr && currentThread == _controlThreadId) {
        return true;
    }
    for (const auto &record : _records) {
        if (record.thread != nullptr && record.thread->threadId() == currentThread) {
            return true;
        }
    }
    return false;
}

auto ApplicationPartManager::state() const noexcept -> ApplicationPartManagerState {
    const auto lock = std::scoped_lock{_mutex};
    return _state;
}

auto ApplicationPartManager::partState(const ApplicationPartIdentifierPtr &identifier) const -> ApplicationPartState {
    const auto lock = std::scoped_lock{_mutex};
    if (!_prepared) {
        throw err::LogicError{"The application-part manager is not prepared."_el};
    }
    return _records[resolveIdentifierLocked(identifier) - 1].state;
}

auto ApplicationPartManager::part(const ApplicationPartIdentifierPtr &identifier) const -> ApplicationPartPtr {
    const auto lock = std::scoped_lock{_mutex};
    if (!_prepared) {
        throw err::LogicError{"The application-part manager is not prepared."_el};
    }
    return _records[resolveIdentifierLocked(identifier) - 1].part;
}

auto ApplicationPartManager::waitForRunning() -> bool {
    auto lock = std::unique_lock{_mutex};
    if (isWaitForbiddenLocked()) {
        throw err::LogicError{"Can not wait for the application-part manager from one of its event threads."_el};
    }
    _stateChanged.wait(lock, [this]() -> bool {
        return _state == ApplicationPartManagerState::Running || _state == ApplicationPartManagerState::Stopping ||
            _state == ApplicationPartManagerState::Stopped || _state == ApplicationPartManagerState::Failed;
    });
    return _state == ApplicationPartManagerState::Running;
}

auto ApplicationPartManager::waitForStopped() -> bool {
    auto lock = std::unique_lock{_mutex};
    if (isWaitForbiddenLocked()) {
        throw err::LogicError{"Can not wait for the application-part manager from one of its event threads."_el};
    }
    _stateChanged.wait(lock, [this]() -> bool {
        return _state == ApplicationPartManagerState::Stopped || _state == ApplicationPartManagerState::Failed;
    });
    return _state == ApplicationPartManagerState::Stopped;
}

auto ApplicationPartManager::waitForRunning(const ApplicationPartIdentifierPtr &identifier) -> bool {
    auto lock = std::unique_lock{_mutex};
    if (!_prepared) {
        throw err::LogicError{"The application-part manager is not prepared."_el};
    }
    if (isWaitForbiddenLocked()) {
        throw err::LogicError{"Can not wait for an application part from one of the manager's event threads."_el};
    }
    const auto number = resolveIdentifierLocked(identifier);
    _stateChanged.wait(lock, [this, number]() -> bool {
        const auto state = _records[number - 1].state;
        return state == ApplicationPartState::Running || state == ApplicationPartState::Stopping ||
            state == ApplicationPartState::Stopped || state == ApplicationPartState::Failed;
    });
    return _records[number - 1].state == ApplicationPartState::Running;
}

auto ApplicationPartManager::waitForStopped(const ApplicationPartIdentifierPtr &identifier) -> bool {
    auto lock = std::unique_lock{_mutex};
    if (!_prepared) {
        throw err::LogicError{"The application-part manager is not prepared."_el};
    }
    if (isWaitForbiddenLocked()) {
        throw err::LogicError{"Can not wait for an application part from one of the manager's event threads."_el};
    }
    const auto number = resolveIdentifierLocked(identifier);
    _stateChanged.wait(lock, [this, number]() -> bool {
        const auto state = _records[number - 1].state;
        return state == ApplicationPartState::Stopped || state == ApplicationPartState::Failed;
    });
    return _records[number - 1].state == ApplicationPartState::Stopped;
}

void ApplicationPartManager::setErrorHandler(ApplicationPartErrorHandler handler) {
    const auto lock = std::scoped_lock{_mutex};
    _errorHandler = std::move(handler);
}

auto ApplicationPartManager::events() noexcept -> core::ApplicationPartManagerEventEditor & {
    return *this;
}

auto ApplicationPartManager::addStateChanged(ApplicationPartManagerStateChangedFn callback)
    -> event::EventSubscription {
    return _stateChangedCallbacks.add(std::move(callback));
}

auto ApplicationPartManager::addPartStateChanged(ApplicationPartStateChangedFn callback) -> event::EventSubscription {
    return _partStateChangedCallbacks.add(std::move(callback));
}

auto ApplicationPartManager::hasError() const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return !_errors.empty();
}

auto ApplicationPartManager::takeError() noexcept -> std::exception_ptr {
    const auto lock = std::scoped_lock{_mutex};
    if (_errors.empty()) {
        return {};
    }
    auto result = _errors.front();
    _errors.pop_front();
    return result;
}

void ApplicationPartManager::invokeControl(std::function<void()> callback) {
    _controlEvents->invoke([weak = weak_from_this(), callback = std::move(callback)]() -> void {
        if (const auto self = weak.lock(); self != nullptr) {
            self->recordControlThread();
            callback();
        }
    });
}

void ApplicationPartManager::recordControlThread() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    _controlThreadId = std::this_thread::get_id();
}

auto ApplicationPartManager::isWaitForbiddenLocked() const noexcept -> bool {
    const auto current = std::this_thread::get_id();
    if (current == _controlThreadId) {
        return true;
    }
    for (const auto &record : _records) {
        if (record.thread != nullptr && record.thread->threadId() == current) {
            return true;
        }
    }
    return false;
}

void ApplicationPartManager::addError(std::exception_ptr error) noexcept {
    if (error == nullptr) {
        return;
    }
    const auto lock = std::scoped_lock{_mutex};
    _errors.emplace_back(std::move(error));
}

void ApplicationPartManager::setManagerState(const ApplicationPartManagerState state) {
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_state == state) {
            return;
        }
        _state = state;
        _stateChanged.notify_all();
    }
    invokeControl([this, state]() -> void {
        _stateChangedCallbacks.notify(
            [this](std::exception_ptr error) -> void { handleCallbackFailure(std::move(error)); }, state);
    });
}

void ApplicationPartManager::setPartState(const std::size_t number, const ApplicationPartState state) {
    auto identifier = ApplicationPartIdentifierPtr{};
    {
        const auto lock = std::scoped_lock{_mutex};
        auto &record = _records[number - 1];
        if (record.state == state) {
            return;
        }
        record.state = state;
        record.part->setState(state);
        identifier = record.identifier;
        _stateChanged.notify_all();
    }
    invokeControl([this, identifier, state]() -> void {
        _partStateChangedCallbacks.notify(
            [this](std::exception_ptr error) -> void { handleCallbackFailure(std::move(error)); }, identifier, state);
    });
}

void ApplicationPartManager::handleCallbackFailure(std::exception_ptr error) {
    addError(std::move(error));
    auto managerState = ApplicationPartManagerState::Uninitialized;
    {
        const auto lock = std::scoped_lock{_mutex};
        _fatalFailure = true;
        managerState = _state;
    }
    if (managerState == ApplicationPartManagerState::Stopped) {
        setManagerState(ApplicationPartManagerState::Failed);
    } else if (managerState != ApplicationPartManagerState::Failed) {
        handleStopAll();
    }
}

}
