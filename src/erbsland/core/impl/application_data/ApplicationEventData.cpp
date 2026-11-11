// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationEventData.hpp"

#include "../../../event/EventLoopErrorAction.hpp"
#include "../../../event/impl/CurrentLoopScope.hpp"
#include "../../../event/impl/ManagedEventThread.hpp"
#include "../../../event/ManagedEventThread.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace erbsland::core::impl {

ApplicationEventData::ApplicationEventData() :
    _eventLoop{std::make_shared<event::impl::EventLoop>()}, _eventRegistry{event::EventRegistry::PrivateTag{}} {
}

auto ApplicationEventData::loop() const noexcept -> const event::EventLoopPtr & {
    return _eventLoop;
}

auto ApplicationEventData::mainEventLoop() noexcept -> event::EventLoop & {
    return *_eventLoop;
}

auto ApplicationEventData::eventRegistry() noexcept -> event::EventRegistry & {
    return _eventRegistry;
}

auto ApplicationEventData::createEventThread(const event::EventLoopOptions &options) -> event::ManagedEventThreadPtr {
    auto result = std::make_shared<event::impl::ManagedEventThread>(
        options, [owner = weak_from_this()](std::exception_ptr error) -> void {
            if (const auto state = owner.lock()) {
                state->recordFailure(std::move(error));
            }
        });
    const auto lock = std::scoped_lock{_mutex};
    std::erase_if(_threads, [](const event::ManagedEventThreadWeakPtr &thread) -> bool { return thread.expired(); });
    _threads.emplace_back(result);
    return result;
}

void ApplicationEventData::runMainEventLoop() {
    _eventLoop->setErrorHandler([this](std::exception_ptr error) -> event::EventLoopErrorAction {
        const auto lock = std::scoped_lock{_mutex};
        if (_loopError == nullptr) {
            _loopError = std::move(error);
        }
        return event::EventLoopErrorAction::Stop;
    });
    auto currentLoopScope = event::impl::CurrentLoopScope{_eventLoop};
    _eventLoop->run();
}

auto ApplicationEventData::isQuitRequested() const noexcept -> bool {
    return _eventLoop->isQuitRequested();
}

auto ApplicationEventData::finishRun() -> unit::ExitCode {
    finishThreads();
    const auto lock = std::scoped_lock{_mutex};
    return _quitExitCodeSet ? _quitExitCode : unit::ExitCode::success();
}

void ApplicationEventData::finishThreads() {
    _eventLoop->setErrorHandler({});
    auto threads = std::vector<event::ManagedEventThreadPtr>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        threads = managedThreads();
    }
    for (const auto &thread : threads) {
        thread->quit();
    }
    for (const auto &thread : threads) {
        thread->join();
    }
    auto loopError = std::exception_ptr{};
    {
        const auto lock = std::scoped_lock{_mutex};
        loopError = _loopError;
    }
    if (loopError != nullptr) {
        std::rethrow_exception(loopError);
    }
}

void ApplicationEventData::recordFailure(std::exception_ptr error) noexcept {
    auto threads = std::vector<event::ManagedEventThreadPtr>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_loopError == nullptr) {
            _loopError = std::move(error);
            static_cast<event::impl::EventLoop &>(*_eventLoop).stopForFailure();
        }
        threads = managedThreads();
    }
    for (const auto &thread : threads) {
        thread->quit();
    }
}

void ApplicationEventData::recordExitCode(const unit::ExitCode exitCode) noexcept {
    const auto lock = std::scoped_lock{_mutex};
    if (!_quitExitCodeSet) {
        _quitExitCode = exitCode;
        _quitExitCodeSet = true;
    }
}

void ApplicationEventData::quit() noexcept {
    auto threads = std::vector<event::ManagedEventThreadPtr>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        threads = managedThreads();
    }
    _eventLoop->quit();
    for (const auto &thread : threads) {
        thread->quit();
    }
}

auto ApplicationEventData::managedThreads() -> std::vector<event::ManagedEventThreadPtr> {
    std::erase_if(_threads, [](const event::ManagedEventThreadWeakPtr &thread) -> bool { return thread.expired(); });
    auto result = std::vector<event::ManagedEventThreadPtr>{};
    for (const auto &thread : _threads) {
        if (auto sharedThread = thread.lock(); sharedThread != nullptr) {
            result.emplace_back(std::move(sharedThread));
        }
    }
    return result;
}

}
