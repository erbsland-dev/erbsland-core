// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CurrentEvents.hpp"

#include "impl/CurrentEventsScope.hpp"

#include "../err/LogicError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::event {

using namespace text::literals;

auto currentEvents() -> EventsPtr {
    const auto result = impl::currentEventsWeakPtr().lock();
    if (result == nullptr) {
        throw err::LogicError{"The current thread is not running an event loop."_el};
    }
    return result;
}

}
