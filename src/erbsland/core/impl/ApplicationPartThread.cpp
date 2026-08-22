// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartThread.hpp"

#include "../../err/LogicError.hpp"
#include "../../event/EventLoop.hpp"
#include "../../event/EventLoopErrorAction.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::core::impl {

using namespace text::literals;

ApplicationPartThread::ApplicationPartThread(
    event::EventCallback startup, event::EventCallback cleanup, ErrorFn errorFn) :
    _eventLoop{event::EventLoop::create()}, _cleanup{std::move(cleanup)}, _errorFn{std::move(errorFn)} {
    _eventLoop->setErrorHandler([errorFn = _errorFn](std::exception_ptr error) -> event::EventLoopErrorAction {
        if (errorFn) {
            errorFn(std::move(error));
        }
        return event::EventLoopErrorAction::Stop;
    });
    _eventLoop->invoke(std::move(startup));
}

ApplicationPartThread::~ApplicationPartThread() noexcept {
    try {
        quit();
        join();
    } catch (...) {
        std::terminate();
    }
}

void ApplicationPartThread::start() {
    auto lock = std::scoped_lock{_mutex};
    if (_started) {
        throw err::LogicError{"The application-part thread was already started."_el};
    }
    _thread = std::thread{[this]() -> void {
        try {
            _eventLoop->run();
        } catch (...) {
            if (_errorFn) {
                _errorFn(std::current_exception());
            }
        }
        _cleanup();
    }};
    _started = true;
}

void ApplicationPartThread::requestStop(event::EventCallback stopping) {
    _eventLoop->invoke(std::move(stopping));
}

void ApplicationPartThread::quit() noexcept {
    _eventLoop->quit();
}

void ApplicationPartThread::join() {
    auto lock = std::scoped_lock{_mutex};
    if (!_thread.joinable()) {
        return;
    }
    if (_thread.get_id() == std::this_thread::get_id()) {
        throw err::LogicError{"Can not join an application-part thread from itself."_el};
    }
    _thread.join();
}

auto ApplicationPartThread::events() const noexcept -> event::EventsPtr {
    return _eventLoop;
}

auto ApplicationPartThread::threadId() const noexcept -> std::thread::id {
    const auto lock = std::scoped_lock{_mutex};
    return _thread.get_id();
}

}
