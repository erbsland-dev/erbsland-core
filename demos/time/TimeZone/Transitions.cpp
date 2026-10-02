// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Resolve folds and gaps using the named zone's rules, including half-hour transitions.
/// Refresh the constructor's display from UTC to check whether requested local fields actually occurred.
/// @notest{Compiled and executed documentation demo.}
void transitions() {
    const auto zone = el::TimeZone::fromNameOrThrow("Australia/Lord_Howe"_el);
    const auto foldDate = el::Date::fromYearMonthDayOrThrow(2026, 4, 5);
    const auto repeated = el::Time{el::Hour{1}, el::Minute{45}};
    const auto first = el::DateTime{foldDate, repeated, zone, el::TimeOccurrenceInFold::First};
    const auto second = el::DateTime{foldDate, repeated, zone, el::TimeOccurrenceInFold::Second};
    el::io::printLine(
        el::StringFormat{"Fold first: {}\nSecond: {}\nElapsed seconds: {}"_el}.build(
            first, second, first.durationTo(second).toSeconds().toRawValue()));
    el::io::printLine(
        el::StringFormat{"UTC first: {}\nUTC second: {}\nDefault chooses first: {}"_el}.build(
            first.toUtc(), second.toUtc(), el::DateTime{foldDate, repeated, zone} == first));

    const auto gapDate = el::Date::fromYearMonthDayOrThrow(2026, 10, 4);
    const auto skipped = el::Time{el::Hour{2}, el::Minute{15}};
    const auto selected = el::DateTime{gapDate, skipped, zone};
    const auto resolved = selected.toUtc().toTimeZone(zone);
    el::io::printLine(
        el::StringFormat{"Gap requested: {} {}\nConstructor: {}\nResolved: {}\nUTC: {}"_el}.build(
            gapDate, skipped, selected, resolved, resolved.toUtc()));
    el::io::printLine(
        el::StringFormat{"Valid: {}\nRequested fields occurred: {}\nGap occurrence choice changes instant: {}"_el}
            .build(
                selected.isValid(),
                resolved.date() == gapDate && resolved.time() == skipped,
                selected.toUtc() != el::DateTime{gapDate, skipped, zone, el::TimeOccurrenceInFold::Second}.toUtc()));
}

}
