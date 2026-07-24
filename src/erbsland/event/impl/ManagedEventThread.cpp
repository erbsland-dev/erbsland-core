// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ManagedEventThread.hpp"

#include "../EventLoop.hpp"

namespace erbsland::event::impl {

ManagedEventThread::~ManagedEventThread() noexcept {
    shutdown();
}

void ManagedEventThread::start() {
    EventThreadBase::start();
}

void ManagedEventThread::quit() noexcept {
    EventThreadBase::quit();
}

void ManagedEventThread::join() {
    EventThreadBase::join();
}

auto ManagedEventThread::isStarted() const noexcept -> bool {
    return EventThreadBase::isStarted();
}

auto ManagedEventThread::isRunning() const noexcept -> bool {
    return EventThreadBase::isRunning();
}

auto ManagedEventThread::eventLoop() -> event::EventLoop & {
    return EventThreadBase::eventLoop();
}

auto ManagedEventThread::events() -> event::EventsPtr {
    return EventThreadBase::events();
}

void ManagedEventThread::runEventLoop() {
    eventLoop().run();
}

}
