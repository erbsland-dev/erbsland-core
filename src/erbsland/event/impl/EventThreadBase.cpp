// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventThreadBase.hpp"

#include "../EventLoop.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::event::impl {

using namespace text::literals;

EventThreadBase::EventThreadBase(const EventLoopOptions &options, EventLoop::FailureHandler failureHandler) :
    _eventLoop{std::make_shared<EventLoop>(options, std::move(failureHandler), _stopSource)} {
}

EventThreadBase::~EventThreadBase() noexcept {
    shutdown();
}

void EventThreadBase::start() {
    std::scoped_lock lock{_mutex};
    if (_started) {
        throw err::LogicFailure{"The event thread was already started."_el};
    }
    auto thread = std::thread{[this]() -> void { runWorkerEventLoop(); }};
    _thread = std::move(thread);
    _started = true;
}

void EventThreadBase::quit() noexcept {
    _eventLoop->quit();
}

auto EventThreadBase::stopToken() const noexcept -> util::StopToken {
    return _stopSource.stopToken();
}

void EventThreadBase::join() {
    std::scoped_lock lock{_mutex};
    if (!_thread.joinable()) {
        return;
    }
    if (_thread.get_id() == std::this_thread::get_id()) {
        throw err::LogicFailure{"Can not join an event thread from itself."_el};
    }
    _thread.join();
}

auto EventThreadBase::isStarted() const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _started;
}

auto EventThreadBase::isRunning() const noexcept -> bool {
    return _eventLoop->isRunning();
}

auto EventThreadBase::workerEventLoop() noexcept -> event::EventLoop & {
    return *_eventLoop;
}

auto EventThreadBase::loop() noexcept -> event::EventTargetPtr {
    return _eventLoop;
}

void EventThreadBase::shutdown() noexcept {
    try {
        auto mustTerminate = false;
        {
            std::scoped_lock lock{_mutex};
            mustTerminate = _thread.joinable() && _thread.get_id() == std::this_thread::get_id();
        }
        if (mustTerminate) {
            std::terminate();
        }
        quit();
        join();
    } catch (...) {
        std::terminate();
    }
}

auto EventThreadBase::eventLoopPtr() const noexcept -> const event::EventLoopPtr & {
    return _eventLoop;
}

}
