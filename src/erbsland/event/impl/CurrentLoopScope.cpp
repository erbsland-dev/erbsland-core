// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CurrentLoopScope.hpp"

#include <utility>

namespace erbsland::event::impl {

thread_local EventTargetWeakPtr CurrentLoopScope::_currentLoop{};

CurrentLoopScope::CurrentLoopScope(EventTargetPtr target) noexcept : _previousLoop{_currentLoop} {
    _currentLoop = std::move(target);
}

CurrentLoopScope::~CurrentLoopScope() noexcept {
    _currentLoop = std::move(_previousLoop);
}

auto CurrentLoopScope::currentLoopWeakPtr() noexcept -> EventTargetWeakPtr {
    return _currentLoop;
}

auto currentLoopWeakPtr() noexcept -> EventTargetWeakPtr {
    return CurrentLoopScope::currentLoopWeakPtr();
}

}
