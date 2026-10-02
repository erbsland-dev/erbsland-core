// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Find leap-year and month lengths and validate a day in its calendar context.
/// @notest{Compiled and executed documentation demo.}
void lengths() {
    const auto year = el::Year{2024};
    const auto month = el::Month::february();
    const auto requested = el::Day{31};
    const auto last = el::Day::last(year, month);
    el::io::printLine(
        el::StringFormat{"Leap year: {}; year days: {}; month days: {}; last day: {}"_el}.build(
            year.isLeapYear(),
            year.dayCount().toRawValue(),
            month.dayCount(year).toRawValue(),
            month.lastDay(year).toValue()));
    el::io::printLine(
        el::StringFormat{"Day 31 exists: {}; clamped: {}; contextual last: {}"_el}.build(
            requested.exists(year, month), requested.clamped(year, month).toValue(), last.isLast(year, month)));
    el::io::printLine(
        el::StringFormat{"February fixed length: {}; min/max days: {}/{}; common-year last ordinal: {}"_el}.build(
            month.hasFixedLength(),
            month.minimumDayCount().toRawValue(),
            month.maximumDayCount().toRawValue(),
            el::DayOfYear::last(el::Year{2023}).toValue()));
}

}
