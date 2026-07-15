// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time::tz::impl {

/// Public-facing generated zone identifier; zero is reserved for UTC.
using ZoneId = uint16_t;

inline constexpr auto cUtcZoneId = ZoneId{0};
inline constexpr auto cZoneIdNotFound = ZoneId{0xffffU};

}
