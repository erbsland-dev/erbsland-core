// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimePartWithAmount.hpp"

#include "../Day_fwd.hpp"
#include "../DayOfWeek_fwd.hpp"
#include "../DayOfYear_fwd.hpp"
#include "../Hour_fwd.hpp"
#include "../Minute_fwd.hpp"
#include "../Month_fwd.hpp"
#include "../Second_fwd.hpp"
#include "../TimeAmounts.hpp"
#include "../Year_fwd.hpp"

namespace erbsland::time::impl {

using YearBase = TimePartWithAmount<Year, Years, int16_t, 0, 9999>;
using MonthBase = TimePartWithAmount<Month, Months, int8_t, 1, 12>;
using DayBase = TimePartWithAmount<Day, Days, int8_t, 1, 31>;
using DayOfWeekBase = TimePartWithAmount<DayOfWeek, Days, int8_t, 0, 6>;
using DayOfYearBase = TimePartWithAmount<DayOfYear, Days, int16_t, 1, 366>;
using HourBase = TimePartWithAmount<Hour, Hours, int8_t, 0, 23>;
using MinuteBase = TimePartWithAmount<Minute, Minutes, int8_t, 0, 59>;
using SecondBase = TimePartWithAmount<Second, Seconds, int8_t, 0, 59>;

}
