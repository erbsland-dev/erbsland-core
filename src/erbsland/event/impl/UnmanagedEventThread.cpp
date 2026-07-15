// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnmanagedEventThread.hpp"

#include "../EventLoop.hpp"

namespace erbsland::event {

auto UnmanagedEventThread::create() -> UnmanagedEventThreadPtr {
    return std::make_shared<impl::UnmanagedEventThread>();
}

}

namespace erbsland::event::impl {

UnmanagedEventThread::~UnmanagedEventThread() noexcept {
    shutdown();
}

void UnmanagedEventThread::start() {
    EventThreadBase::start();
}

void UnmanagedEventThread::quit() noexcept {
    EventThreadBase::quit();
}

void UnmanagedEventThread::join() {
    EventThreadBase::join();
}

auto UnmanagedEventThread::isStarted() const noexcept -> bool {
    return EventThreadBase::isStarted();
}

auto UnmanagedEventThread::isRunning() const noexcept -> bool {
    return EventThreadBase::isRunning();
}

auto UnmanagedEventThread::eventLoop() -> event::EventLoop & {
    return EventThreadBase::eventLoop();
}

auto UnmanagedEventThread::events() -> event::EventsPtr {
    return EventThreadBase::events();
}

void UnmanagedEventThread::runEventLoop() {
    eventLoop().run();
}

}
