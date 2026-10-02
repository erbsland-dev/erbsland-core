// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Resolve UTC, fixed-offset, and named-zone calendar fields into dated instants.
/// @notest{Compiled and executed documentation demo.}
void create() {
    const auto lesson = "Μάθημα μαγείας"_el;
    const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 1);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}};
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto utc = el::DateTime{date, time};
    const auto fixedSeconds = el::DateTime{date, time, el::Seconds{10'800}};
    const auto fixedDuration = el::DateTime{date, time, el::Duration{el::Hours{3}}};
    const auto named = el::DateTime{date, time, zone};
    const auto zonedTime = el::DateTime{date, el::TimeWithZone{time, zone}};
    el::io::printLine(
        el::StringFormat{"Lesson: {}; UTC input: {}; named local input: {}"_el}.build(lesson, utc, named));
    el::io::printLine(
        el::StringFormat{"Fixed constructors agree: {}; zoned-time constructor agrees: {}"_el}.build(
            fixedSeconds == fixedDuration, named == zonedTime));
    el::io::printLine(
        el::StringFormat{"Default valid: {}; invalid date valid: {}; current clock valid: {}"_el}.build(
            el::DateTime{}.isValid(), el::DateTime{el::Date{}, time, zone}.isValid(), el::DateTime::now().isValid()));
    el::io::printLine(
        el::StringFormat{"Core epoch: {}; POSIX epoch: {}; first: {}; last: {}"_el}.build(
            el::DateTime::epoch(),
            el::DateTime::epoch(el::TimeEpoch::Posix),
            el::DateTime::first(),
            el::DateTime::last()));
}

}
