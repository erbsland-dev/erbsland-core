// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Choose a fold occurrence and check a gap by resolving the selected instant back into its zone.
/// A valid DateTime can still originate from a local reading that never occurred.
/// @notest{Compiled and executed documentation demo.}
void transitions() {
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto rehearsal = el::TimeWithZone{el::Time{el::Hour{3}, el::Minute{30}}, zone};
    const auto foldDate = el::Date::fromYearMonthDayOrThrow(2026, 10, 25);
    const auto first = el::DateTime{foldDate, rehearsal, el::TimeOccurrenceInFold::First};
    const auto second = el::DateTime{foldDate, rehearsal, el::TimeOccurrenceInFold::Second};
    el::io::printLine(
        el::StringFormat{"Fold first UTC: {}\nSecond UTC: {}\nDefault chooses first: {}"_el}.build(
            first.toUtc(), second.toUtc(), el::DateTime{foldDate, rehearsal} == first));

    // Refresh display metadata from UTC before comparing the requested local fields.
    const auto gapDate = el::Date::fromYearMonthDayOrThrow(2026, 3, 29);
    const auto selected = el::DateTime{gapDate, rehearsal};
    const auto resolved = selected.toUtc().toTimeZone(zone);
    el::io::printLine(
        el::StringFormat{"Gap requested: {} {}\nConstructor: {}\nResolved: {}"_el}.build(
            gapDate, rehearsal.time(), selected, resolved));
    el::io::printLine(
        el::StringFormat{"Gap valid: {}\nRequested fields occurred: {}\nInvalid date accepted: {}"_el}.build(
            selected.isValid(),
            resolved.date() == gapDate && resolved.time() == rehearsal.time(),
            el::DateTime{el::Date{}, rehearsal}.isValid()));
}

}
