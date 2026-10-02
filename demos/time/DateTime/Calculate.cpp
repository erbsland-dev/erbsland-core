// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Apply fixed or calendar changes in UTC and detect boundaries before updating an instant.
/// @notest{Compiled and executed documentation demo.}
void calculate() {
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto start =
        el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 3, 28), el::Time{el::Hour{12}, el::Minute{0}}, zone};
    const auto next = start.addedOrThrow(el::Duration{el::Hours{24}});
    el::io::printLine(
        el::StringFormat{"Start: {}; 24 hours later: {}; elapsed (seconds): {}"_el}.build(
            start, next, start.durationTo(next).toSeconds().toRawValue()));
    auto adjusted = next;
    adjusted.subtractOrThrow(el::Duration{el::Hours{24}});
    adjusted.add(el::Duration{el::Seconds{1}});
    adjusted.subtract(el::Duration{el::Seconds{1}});
    el::io::printLine(el::StringFormat{"In-place round trip: {}"_el}.build(adjusted == start));
    const auto monthEnd = el::DateTime::fromIsoStringOrThrow("2028-01-31T12:00:00Z"_el);
    const auto shifted = monthEnd.addedOrThrow(el::CalendarDelta{el::Months{1}});
    el::io::printLine(
        el::StringFormat{"One month later: {}; one month back: {}"_el}.build(
            shifted, shifted.subtractedOrThrow(el::CalendarDelta{el::Months{1}})));
    const auto precise = monthEnd.addedOrThrow(el::CalendarDelta{el::Nanoseconds{250'000'000}});
    el::io::printLine(
        el::StringFormat{"Fractional shift: {}; DateTime delta: {}; Timestamp delta: {}"_el}.build(
            precise,
            monthEnd.timeDeltaTo(precise),
            el::Timestamp::fromDateTimeOrThrow(monthEnd).timeDeltaToOrThrow(
                el::Timestamp::fromDateTimeOrThrow(precise))));
    el::io::printLine(
        el::StringFormat{"Last plus second saturates: {}; first minus second saturates: {}; clamped: {}"_el}.build(
            el::DateTime::last().wouldAddSaturate(el::Duration{el::Seconds{1}}),
            el::DateTime::first().wouldSubtractSaturate(el::Duration{el::Seconds{1}}),
            el::DateTime::last().added(el::Duration{el::Seconds{1}}) == el::DateTime::last()));
    try {
        const auto unexpected = el::DateTime::last().addedOrThrow(el::CalendarDelta{el::Nanoseconds{1}});
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
    } catch (const el::err::OverflowError &) {
        el::io::printLine("Checked calendar arithmetic rejects a boundary crossing."_el);
    }
}

}
