// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Resolve ordinal dates and keep one-based day labels separate from zero-based offsets.
/// @notest{Compiled and executed documentation demo.}
void ordinal() {
    const auto year = el::Year{2024};
    const auto ordinal = el::DayOfYear{60};
    const auto monthDay = el::Month::extractMonthAndDay(year, ordinal);
    const auto date = el::Date::fromParts(year, monthDay.month, monthDay.day);
    const auto extracted = el::Year::extractFromEpoch(date.toDaysSinceEpoch());
    const auto fromOffset = el::Month::extractMonthAndDay(extracted.year, extracted.dayOfYear);
    el::io::printLine(
        el::StringFormat{"Ordinal {}: {}; month from year: {}; zero-based offset: {}; recovered day: {}"_el}.build(
            ordinal.toValue(),
            date.toString(),
            year.monthOfDay(ordinal).toValue(),
            extracted.dayOfYear.toRawValue(),
            fromOffset.day.toValue()));
    const auto march = el::Month::march();
    el::io::printLine(
        el::StringFormat{"Days before March: {}; March ordinals: {}..{}; year-end ordinal is last: {}"_el}.build(
            year.daysBeforeMonth(march).toRawValue(),
            march.firstDayOfYear(year).toValue(),
            march.lastDayOfYear(year).toValue(),
            el::DayOfYear::last(year).isLast(year)));
    el::io::printLine(
        el::StringFormat{"366 in part range: {}; last ordinal in 2023: {}"_el}.build(
            el::DayOfYear::contains(366), el::DayOfYear::last(el::Year{2023}).toValue()));
}

}
