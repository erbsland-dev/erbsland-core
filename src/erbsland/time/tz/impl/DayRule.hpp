// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time::tz::impl {

/// Rule used to resolve the day of a transition.
/// @notest{Internal generated-data helper.}
enum class DayRule : uint8_t {
    Exact,
    AtOrLater,
    AtOrEarlier,
    Last,
};

}
