// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Find a weekday on or after a date, or on or before it, with signed day offsets.
/// @notest{Compiled and executed documentation demo.}
void weekdays() {
    const auto date = el::Date{el::Year{2024}, el::Month{6}, el::Day{12}};
    const auto weekday = date.dayOfWeek();
    const auto target = el::DayOfWeek::monday();
    const auto forward = weekday.daysToNext(target);
    const auto backward = weekday.daysToPrevious(target);
    el::io::printLine(
        el::StringFormat{"{}: {} / {}; Monday is {}"_el}.build(
            date.toString(), weekday.toString(), weekday.toString(el::DayOfWeekFormat::Short), target.toValue()));
    el::io::printLine(
        el::StringFormat{"Next Monday: {} ({} days); previous Monday: {} ({} days)"_el}.build(
            date.added(forward).toString(),
            forward.toRawValue(),
            date.added(backward).toString(),
            backward.toRawValue()));
    el::io::printLine(
        el::StringFormat{"Equal weekday next/previous: {}/{} days"_el}.build(
            target.daysToNext(target).toRawValue(), target.daysToPrevious(target).toRawValue()));
}

}
