// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Resolve a stored wall-clock time separately for each chosen local date.
/// Named-zone rules can give the same reading different UTC offsets in winter and summer.
/// @notest{Compiled and executed documentation demo.}
void resolve() {
    const auto movement = "Αντάντε"_el;
    const auto rehearsal =
        el::TimeWithZone{el::Time{el::Hour{9}, el::Minute{15}}, el::TimeZone::fromNameOrThrow("Europe/Athens"_el)};
    for (
        const auto date :
        {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), el::Date::fromYearMonthDayOrThrow(2026, 7, 15)}) {
        // The Date is local input; DateTime stores the resulting UTC instant.
        const auto occurrence = el::DateTime{date, rehearsal};
        el::io::printLine(
            el::StringFormat{"{}: local {}\nUTC {}\nOffset seconds: {}"_el}.build(
                movement, occurrence, occurrence.toUtc(), occurrence.timeOffset().toSeconds().toRawValue()));
    }
}

}
