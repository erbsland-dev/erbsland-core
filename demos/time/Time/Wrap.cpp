// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Carry crossed days when an observation continues past midnight.
/// @notest{Compiled and executed documentation demo.}
void wrap() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto start = el::Time{el::Hour{23}, el::Minute{45}};
    const auto finished = start.addedWithWrap(el::Duration{el::Minutes{90}});
    el::io::printLine(
        el::StringFormat{"Finish: {} {}; day carry: {}"_el}.build(
            date.addedOrThrow(finished.days), finished.time, finished.days.toRawValue()));
    el::io::printLine(el::StringFormat{"Start remains: {}"_el}.build(start));

    auto revised = start;
    const auto days = revised.addWithWrap(el::Duration{el::Hours{49}});
    el::io::printLine(el::StringFormat{"49 hours later: {}; day carry: {}"_el}.build(revised, days.toRawValue()));
    const auto previous = el::Time{}.addedWithWrap(el::TimeDelta{el::Nanoseconds{-1}});
    el::io::printLine(
        el::StringFormat{"One nanosecond before midnight: {}; day carry: {}"_el}.build(
            previous.time, previous.days.toRawValue()));
    auto midnight = el::Time{};
    const auto previousDays = midnight.addWithWrap(el::TimeDelta{el::Minutes{-90}});
    el::io::printLine(
        el::StringFormat{"Minus 90 minutes in place: {}; day carry: {}"_el}.build(midnight, previousDays.toRawValue()));
}

}
