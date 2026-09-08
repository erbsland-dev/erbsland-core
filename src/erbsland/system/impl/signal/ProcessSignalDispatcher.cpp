// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProcessSignalDispatcher.hpp"

#include "ProcessSignalBackend.hpp"

#include <exception>
#include <utility>

namespace erbsland::system::impl {

auto ProcessSignalDispatcher::instance() -> ProcessSignalDispatcher & {
    static auto dispatcher = ProcessSignalDispatcher{};
    return dispatcher;
}

ProcessSignalDispatcher::ProcessSignalDispatcher() :
    _backend{ProcessSignalBackend::create([this](const ProcessSignal signal) -> void { handleSignal(signal); })} {
}

ProcessSignalDispatcher::~ProcessSignalDispatcher() = default;

auto ProcessSignalDispatcher::addSignal(SignalFn signalFn) -> event::EventSubscription {
    return _callbacks.add(std::move(signalFn));
}

void ProcessSignalDispatcher::handleSignal(const ProcessSignal signal) noexcept {
    auto claimed = false;
    _callbacks.notify([](std::exception_ptr) noexcept -> void {}, signal, claimed);
    if (!claimed) {
        _backend->terminateWithDefault(signal);
    }
}

}
