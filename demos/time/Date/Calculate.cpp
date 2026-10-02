// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Move observation dates by days, months, or years.
/// @notest{Compiled and executed documentation demo.}
void calculate() {
    const auto observation = el::Date::fromYearMonthDayOrThrow(2028, 1, 31);
    const auto nextMonth = observation.added(el::Months{1});
    el::io::printLine(
        el::StringFormat{"One month later: {}; one month back: {}"_el}.build(
            nextMonth, nextMonth.added(el::Months{-1})));
    el::io::printLine(el::StringFormat{"Next year from leap day: {}"_el}.build(nextMonth.addedOrThrow(el::Years{1})));
    el::io::printLine(el::StringFormat{"Seven days later: {}"_el}.build(observation.addedOrThrow(el::Days{7})));

    auto revised = observation;
    revised.add(el::Months{1});
    revised.addOrThrow(el::Days{2});
    revised.addOrThrow(el::Years{1});
    el::io::printLine(el::StringFormat{"Revised date: {}"_el}.build(revised));
    el::io::printLine(
        el::StringFormat{"Previous day: {}; next day: {}"_el}.build(observation.previous(), observation.next()));
    el::io::printLine(
        el::StringFormat{"Next Friday: {}; previous Friday: {}"_el}.build(
            observation.next(el::DayOfWeek::friday()), observation.previous(el::DayOfWeek::friday())));
    el::io::printLine(
        el::StringFormat{"Days forward: {}; days backward: {}"_el}.build(
            observation.daysTo(nextMonth).toRawValue(), nextMonth.daysTo(observation).toRawValue()));
}

}
