// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PipeBackend.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/EventBackendTarget.hpp>
#include <erbsland/LogicFailure.hpp>

namespace demo {

using namespace el::text::literals;

PipeBackend::PipeBackend(el::EventRegistry &registry, std::function<void(const el::ByteBlock &)> consume) :
    _id{registry.registerBackend("dev.erbsland.demo.PipeLog"_el)},
    _inputEvent{registry.registerEvent("dev.erbsland.demo.LogChunk"_el)},
    _consume{std::move(consume)} {
}

auto PipeBackend::backendId() const noexcept -> el::EventBackendId {
    return _id;
}

void PipeBackend::attach(el::EventBackendTargetWeakPtr target, el::EventLoopDriverWeakPtr driver) {
    _target = std::move(target);
    _driver = std::dynamic_pointer_cast<PosixPipeDriver>(driver.lock());
    if (!_driver) {
        throw el::LogicFailure{"PipeBackend requires a PosixPipeDriver."_el};
    }
}

void PipeBackend::poll(el::TimePoint) {
    if (_pending) {
        return;
    }
    _pending = _driver->readReadyChunk();
    if (_pending) {
        const auto target = _target.lock();
        if (!target || !target->postFromBackend(el::Event{_inputEvent})) {
            throw el::ApplicationError{"Failed to queue log input."_el};
        }
    }
}

auto PipeBackend::handleEvent(const el::Event &event) -> bool {
    if (event.identifier() != _inputEvent) {
        return false;
    }
    if (!_pending) {
        throw el::LogicFailure{"Log event without pending input."_el};
    }
    _consume(*_pending);
    _pending.reset();
    return true;
}

auto PipeBackend::nextWakeTime() const -> std::optional<el::TimePoint> {
    return {};
}

}
