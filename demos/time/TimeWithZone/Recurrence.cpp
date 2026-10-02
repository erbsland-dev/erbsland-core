// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Preserve an intended local time by resolving it anew on each local calendar date.
/// Adding a fixed elapsed day can change the displayed time across a zone transition.
/// @notest{Compiled and executed documentation demo.}
void recurrence() {
    const auto movement = "Αντάντε"_el;
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto rehearsal = el::TimeWithZone{el::Time{el::Hour{9}, el::Minute{15}}, zone};
    auto localDate = el::Date::fromYearMonthDayOrThrow(2026, 3, 28);
    const auto first = el::DateTime{localDate, rehearsal};
    for (auto day = 0; day < 3; ++day) {
        const auto selected = el::DateTime{localDate, rehearsal, el::TimeOccurrenceInFold::First};
        const auto resolved = selected.toUtc().toTimeZone(zone);
        const auto fieldsMatch =
            resolved.isValid() && resolved.date() == localDate && resolved.time() == rehearsal.time();
        el::io::printLine(
            el::StringFormat{"{}: {}\nRequested fields retained: {}"_el}.build(movement, resolved, fieldsMatch));
        if (!fieldsMatch) {
            // This recurrence rejects a skipped reading instead of silently changing its time.
            el::io::printLine("Occurrence needs a gap policy\nStop generating dates."_el);
            return;
        }
        localDate = localDate.next();
    }
    const auto fixedDayLater = first.added(el::Duration{el::Days{1}});
    const auto nextLocalDate = el::DateTime{first.date().next(), rehearsal};
    el::io::printLine(
        el::StringFormat{"Fixed day later: {}\nNext local occurrence: {}\nElapsed seconds: {}"_el}.build(
            fixedDayLater, nextLocalDate, first.durationTo(nextLocalDate).toSeconds().toRawValue()));
}

}
