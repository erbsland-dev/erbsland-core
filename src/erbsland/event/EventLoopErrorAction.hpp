// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::event {

/// The action to take after an event-loop error.
enum class EventLoopErrorAction : uint8_t {
    /// Continue the event loop.
    Continue,
    /// Stop this event loop.
    Stop,
};

}
