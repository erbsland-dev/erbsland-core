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

}
