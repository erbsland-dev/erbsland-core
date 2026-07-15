// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time {

/// Controls which part is used as the largest unit when splitting a duration into parts.
///
/// For example, `DurationPart::Days` causes `Duration::parts()` to return days as the
/// largest unit, with weeks zeroed out.
enum class DurationPart : uint8_t {
    Seconds, ///< The seconds part.
    Minutes, ///< The minutes part.
    Hours,   ///< The hours part.
    Days,    ///< The days part.
    Weeks,   ///< The weeks part.
};

}
