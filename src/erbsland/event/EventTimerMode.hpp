// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::event {

/// The current scheduling mode of an event timer.
enum class EventTimerMode : uint8_t {
    Inactive,   ///< The timer is inactive.
    Once,       ///< The timer runs once.
    FixedDelay, ///< The timer repeats with a delay after each callback.
    FixedRate,  ///< The timer repeats on a fixed cadence.
};

}
