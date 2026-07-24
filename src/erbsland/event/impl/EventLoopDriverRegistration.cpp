// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoopDriverRegistration.hpp"

namespace erbsland::event::impl {

EventLoopDriverRegistration::EventLoopDriverRegistration(std::function<void()> unregister) :
    _unregister{std::move(unregister)} {
}

EventLoopDriverRegistration::~EventLoopDriverRegistration() {
    reset();
}

void EventLoopDriverRegistration::reset() noexcept {
    if (!_unregister) {
        return;
    }
    auto unregister = std::move(_unregister);
    try {
        unregister();
    } catch (...) {}
}

auto EventLoopDriverRegistration::isRegistered() const noexcept -> bool {
    return static_cast<bool>(_unregister);
}

}
