// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkerPool.hpp"

#include "impl/worker_pool/WorkerPoolState.hpp"

#include "../event/CurrentEvents.hpp"

#include <utility>

namespace erbsland::util {

WorkerPool::WorkerPool(const WorkerPoolOptions &options) : _state{std::make_shared<impl::WorkerPoolState>(options)} {
    _state->start();
}

WorkerPool::~WorkerPool() {
    _state->shutdown();
}

auto WorkerPool::create(const WorkerPoolOptions &options) -> WorkerPoolPtr {
    return WorkerPoolPtr{new WorkerPool{options}};
}

auto WorkerPool::workerCount() const noexcept -> unsigned int {
    return _state->workerCount();
}

auto WorkerPool::isAccepting() const noexcept -> bool {
    return _state->isAccepting();
}

auto WorkerPool::stopToken() const noexcept -> StopToken {
    return _state->stopToken();
}

void WorkerPool::wait() {
    _state->wait();
}

void WorkerPool::quit() noexcept {
    _state->quit();
}

void WorkerPool::stop() noexcept {
    _state->stop();
}

void WorkerPool::join() {
    _state->join();
}

void WorkerPool::enqueue(impl::WorkerTaskPtr task) {
    _state->enqueue(std::move(task));
}

void WorkerPool::prepareEvents() {
    if (ownerEvents() == nullptr) {
        bindEvents(event::currentEvents());
    }
    verifyCurrentOwnerEvents();
}

void WorkerPool::bindEvents(const event::EventsPtr &owner) {
    _state->bind(owner, shared_from_this());
    bindOwnerEvents(owner);
}

auto WorkerPool::events() -> WorkerPoolEventEditor & {
    prepareEvents();
    const auto owner = ownerEvents();
    if (!_editor) {
        _editor.reset(new WorkerPoolEventEditor{shared_from_this(), owner, _state});
    }
    return *_editor;
}

}
