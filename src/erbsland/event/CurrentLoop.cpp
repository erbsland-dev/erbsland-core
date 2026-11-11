// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CurrentLoop.hpp"

#include "impl/CurrentLoopScope.hpp"

#include "../err/LogicFailure.hpp"
#include "../text/Literals.hpp"

namespace erbsland::event {

using namespace text::literals;

auto currentLoop() -> EventTargetPtr {
    const auto result = impl::currentLoopWeakPtr().lock();
    if (result == nullptr) {
        throw err::LogicFailure{"The current thread is not running an event loop."_el};
    }
    return result;
}

}
