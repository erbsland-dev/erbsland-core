// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time::tz::impl {

/// Time reference used by a generated DST rule.
/// @notest{Internal generated-data helper.}
enum class RuleAtTimeZone : uint8_t {
    Utc,
    WallClock,
    StandardTime,
};

}
