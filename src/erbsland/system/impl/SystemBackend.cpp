// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SystemBackend.hpp"

#include "subprocess/SubprocessState.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../event/CurrentEvents.hpp"
#include "../../event/Events.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::system::impl {

using namespace text::literals;

auto SystemBackend::backendId() const noexcept -> event::EventBackendId {
    return event::id::SystemBackend;
}

void SystemBackend::attach(event::EventBackendTargetWeakPtr target, event::EventLoopDriverWeakPtr) {
    _target = std::move(target);
}

void SystemBackend::poll(time::TimePoint) {
    // Copy weak registrations: callbacks may create more sources and invalidate the registry storage.
    const auto sources = _sources;
    for (const auto &entry : sources) {
        if (const auto source = entry.lock()) {
            source->_state->queueDispatch(source->ownerEvents(), source->_state);
        }
    }
    std::erase_if(_sources, [](const std::weak_ptr<Subprocess> &entry) -> bool { return entry.expired(); });
}

auto SystemBackend::handleEvent(const event::Event &) -> bool {
    return false;
}

auto SystemBackend::nextWakeTime() const -> std::optional<time::TimePoint> {
    return {};
}

auto SystemBackend::createSubprocess(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
    -> SubprocessPtr {
    auto process = Subprocess::create(executable, arguments, options);
    bindSubprocess(process);
    return process;
}

void SystemBackend::bindSubprocess(const SubprocessPtr &process) {
    const auto owner = std::dynamic_pointer_cast<event::Events>(_target.lock());
    if (owner == nullptr || owner != event::currentEvents()) {
        throw err::LogicFailure{"System event sources must be configured on their owner loop."_el};
    }
    process->bindEvents(owner);
    process->_state->bind(_target);
    _sources.emplace_back(process);
}

}
