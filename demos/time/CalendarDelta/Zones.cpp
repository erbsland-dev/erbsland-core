// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Distinguish UTC calendar arithmetic from resolving a recurring local time.
/// @notest{Compiled and executed documentation demo.}
void zones() {
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Copenhagen"_el);
    const auto wallTime = el::TimeWithZone{el::Time{el::Hour{9}, el::Minute{}}, zone};
    const auto start = el::DateTime{el::Date{el::Year{2024}, el::Month{3}, el::Day{30}}, wallTime};
    const auto advanced = start.added(el::CalendarDelta{el::Days{1}});
    // Resolve the intended local time again on the next local date.
    const auto recurring = el::DateTime{start.date().next(), wallTime, el::TimeOccurrenceInFold::First};
    el::io::printLine(
        el::StringFormat{"Start: {}; UTC: {} {}"_el}.build(
            start.toString(), start.utcDate().toString(), start.utcTime().toString()));
    el::io::printLine(
        el::StringFormat{"UTC day added: {}; offset seconds: {}"_el}.build(
            advanced.toString(), advanced.timeOffset().toSeconds().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Next local occurrence: {}; requested hour retained: {}"_el}.build(
            recurring.toString(), recurring.hour() == wallTime.hour()));
}

}
