// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UtilitiesBackend.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../event/CurrentLoop.hpp"
#include "../../event/EventTarget.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::util::impl {

using namespace text::literals;

auto UtilitiesBackend::backendId() const noexcept -> event::EventBackendId {
    return event::id::UtilitiesBackend;
}

void UtilitiesBackend::attach(event::EventBackendTargetWeakPtr target, event::EventLoopDriverWeakPtr) {
    _target = std::move(target);
}

void UtilitiesBackend::poll(time::TimePoint) {
}

auto UtilitiesBackend::handleEvent(const event::Event &) -> bool {
    return false;
}

auto UtilitiesBackend::nextWakeTime() const -> std::optional<time::TimePoint> {
    return {};
}

auto UtilitiesBackend::createWorkerPool(const WorkerPoolOptions &options) -> WorkerPoolPtr {
    const auto owner = std::dynamic_pointer_cast<event::EventTarget>(_target.lock());
    if (owner == nullptr || owner != event::currentLoop()) {
        throw err::LogicFailure{"Utility event sources must be configured on their owner loop."_el};
    }
    auto pool = WorkerPool::create(options);
    pool->bindEvents(owner);
    return pool;
}

}
