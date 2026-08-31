// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnmanagedEventThread.hpp"

#include "../EventLoop.hpp"

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

auto UnmanagedEventThread::eventLoop() noexcept -> event::EventLoop & {
    return EventThreadBase::eventLoop();
}

auto UnmanagedEventThread::events() noexcept -> event::EventsPtr {
    return EventThreadBase::events();
}

void UnmanagedEventThread::runEventLoop() {
    eventLoop().run();
}

}
