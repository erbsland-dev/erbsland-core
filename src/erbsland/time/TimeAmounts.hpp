// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeUnitTags.hpp"

#include "../unit/IntegerAmount.hpp"

#include <ratio>

namespace erbsland::time {

/// Nanosecond amount.
using Nanoseconds = unit::IntegerAmount<SecondsUnitTag, std::nano>;
/// Microsecond amount.
using Microseconds = unit::IntegerAmount<SecondsUnitTag, std::micro>;
/// Millisecond amount.
using Milliseconds = unit::IntegerAmount<SecondsUnitTag, std::milli>;
/// Second amount.
using Seconds = unit::IntegerAmount<SecondsUnitTag, std::ratio<1>>;
/// Minute amount.
using Minutes = unit::IntegerAmount<SecondsUnitTag, std::ratio<60>>;
/// Hour amount.
using Hours = unit::IntegerAmount<SecondsUnitTag, std::ratio<3600>>;
/// Day amount.
using Days = unit::IntegerAmount<SecondsUnitTag, std::ratio<86400>>;
/// Week amount.
using Weeks = unit::IntegerAmount<SecondsUnitTag, std::ratio<604800>>;
/// Month amount.
using Months = unit::IntegerAmount<MonthsUnitTag, std::ratio<1>>;
/// Year amount.
using Years = unit::IntegerAmount<YearsUnitTag, std::ratio<1>>;

namespace literals {

/// Literal for nanoseconds.
inline auto operator_ns(const int64_t value) -> Nanoseconds {
    return Nanoseconds{value};
}
/// Literal for microseconds.
inline auto operator_us(const int64_t value) -> Microseconds {
    return Microseconds{value};
}
/// Literal for milliseconds.
inline auto operator_ms(const int64_t value) -> Milliseconds {
    return Milliseconds{value};
}
/// Literal for seconds.
inline auto operator_s(const int64_t value) -> Seconds {
    return Seconds{value};
}
/// Literal for minutes.
inline auto operator_m(const int64_t value) -> Minutes {
    return Minutes{value};
}
/// Literal for hours.
inline auto operator_h(const int64_t value) -> Hours {
    return Hours{value};
}

}

}
