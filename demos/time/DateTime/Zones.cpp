// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Resolve repeated or missing local readings and refresh named-zone display metadata.
/// @notest{Compiled and executed documentation demo.}
void zones() {
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto foldDate = el::Date::fromYearMonthDayOrThrow(2026, 10, 25);
    const auto reading = el::Time{el::Hour{3}, el::Minute{30}};
    const auto first = el::DateTime{foldDate, reading, zone, el::TimeOccurrenceInFold::First};
    const auto second = el::DateTime{foldDate, reading, zone, el::TimeOccurrenceInFold::Second};
    el::io::printLine(
        el::StringFormat{"First occurrence: {}; second occurrence: {}; apart (seconds): {}"_el}.build(
            first, second, first.durationTo(second).toSeconds().toRawValue()));

    // Re-resolve the selected UTC instant to check a requested wall-clock reading.
    const auto gapDate = el::Date::fromYearMonthDayOrThrow(2026, 3, 29);
    const auto gap = el::DateTime{gapDate, reading, zone};
    const auto resolved = gap.toUtc().toTimeZone(zone);
    el::io::printLine(
        el::StringFormat{"Gap constructor display: {}; refreshed display: {}; valid: {}"_el}.build(
            gap, resolved, gap.isValid()));
    el::io::printLine(
        el::StringFormat{"Requested local fields retained after resolution: {}"_el}.build(
            resolved.date() == gapDate && resolved.time() == reading));
    el::io::printLine(
        el::StringFormat{"Unknown zone accepted: {}; invalid remains invalid on zone conversion: {}"_el}.build(
            el::TimeZone::fromName("Unknown/Academy"_el).has_value(), !el::DateTime{}.toTimeZone(zone).isValid()));
}

}
