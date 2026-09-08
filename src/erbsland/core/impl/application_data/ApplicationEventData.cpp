// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationEventData.hpp"

#include "../../../event/EventLoopErrorAction.hpp"
#include "../../../event/impl/CurrentEventsScope.hpp"
#include "../../../event/impl/ManagedEventThread.hpp"
#include "../../../event/ManagedEventThread.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace erbsland::core::impl {

ApplicationEventData::ApplicationEventData() :
    _eventLoop{std::make_shared<event::impl::EventLoop>()}, _eventRegistry{event::EventRegistry::PrivateTag{}} {
}

auto ApplicationEventData::events() const noexcept -> const event::EventLoopPtr & {
    return _eventLoop;
}

auto ApplicationEventData::eventLoop() noexcept -> event::EventLoop & {
    return *_eventLoop;
}

auto ApplicationEventData::eventRegistry() noexcept -> event::EventRegistry & {
    return _eventRegistry;
}

auto ApplicationEventData::createEventThread() -> event::ManagedEventThreadPtr {
    auto result = std::make_shared<event::impl::ManagedEventThread>();
    const auto lock = std::scoped_lock{_mutex};
    std::erase_if(_threads, [](const event::ManagedEventThreadWeakPtr &thread) -> bool { return thread.expired(); });
    _threads.emplace_back(result);
    return result;
}

void ApplicationEventData::runLoop() {
    _eventLoop->setErrorHandler([this](std::exception_ptr error) -> event::EventLoopErrorAction {
        const auto lock = std::scoped_lock{_mutex};
        if (_loopError == nullptr) {
            _loopError = std::move(error);
        }
        return event::EventLoopErrorAction::Stop;
    });
    auto currentEventsScope = event::impl::CurrentEventsScope{_eventLoop};
    _eventLoop->run();
}

auto ApplicationEventData::isQuitRequested() const noexcept -> bool {
    return _eventLoop->isQuitRequested();
}

auto ApplicationEventData::finishRun() -> unit::ExitCode {
    _eventLoop->setErrorHandler({});
    auto exitCode = unit::ExitCode::success();
    auto loopError = std::exception_ptr{};
    auto threads = std::vector<event::ManagedEventThreadPtr>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_quitExitCodeSet) {
            exitCode = _quitExitCode;
        }
        loopError = _loopError;
        threads = managedThreads();
    }
    for (const auto &thread : threads) {
        thread->quit();
    }
    for (const auto &thread : threads) {
        thread->join();
    }
    if (loopError != nullptr) {
        std::rethrow_exception(loopError);
    }
    return exitCode;
}

void ApplicationEventData::recordExitCode(const unit::ExitCode exitCode) noexcept {
    try {
        const auto lock = std::scoped_lock{_mutex};
        if (!_quitExitCodeSet) {
            _quitExitCode = exitCode;
            _quitExitCodeSet = true;
        }
    } catch (...) {}
}

void ApplicationEventData::quit() noexcept {
    try {
        auto threads = std::vector<event::ManagedEventThreadPtr>{};
        {
            const auto lock = std::scoped_lock{_mutex};
            threads = managedThreads();
        }
        _eventLoop->quit();
        for (const auto &thread : threads) {
            thread->quit();
        }
    } catch (...) {}
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
