// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time {

/// Formatting style for day-of-week names.
///
/// Used by `DayOfWeek::toString()` to control the output format.
enum class DayOfWeekFormat : uint8_t {
    Short,
    Long,
};

}
