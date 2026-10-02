// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Use the cached system-local base zone while resolving its offset separately for each date.
/// Copies and arithmetic preserve local origin; conversion adopts the target zone's origin.
/// @notest{Compiled and executed documentation demo.}
void local() {
    const auto zone = el::TimeZone::local();
    const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 15);
    const auto value = el::DateTime{date, el::Time{el::Hour{9}, el::Minute{15}}, zone};
    // Reconstruct the same base zone explicitly to distinguish identity from local origin.
    const auto explicitZone = zone.isNamed() ? el::TimeZone{zone.id()} : el::TimeZone{zone.staticOffset()};
    el::io::printLine(el::StringFormat{"Local zone equals explicit base zone: {}"_el}.build(zone == explicitZone));
    const auto copy = value;
    const auto later = value.added(el::Duration{el::Days{1}});
    const auto utc = value.toTimeZone(el::TimeZone::utc());
    const auto returned = utc.toTimeZone(zone);
    el::io::printLine(
        el::StringFormat{"Cached zone equal: {}\nLocal origin: {}\nDefault display: {}"_el}.build(
            zone == el::TimeZone::local(), value.isLocalTime(), value.toString()));
    const auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimeShift};
    el::io::printLine(el::StringFormat{"Explicit numeric shift: {}"_el}.build(value.toIsoString(format)));
    el::io::printLine(
        el::StringFormat{"Copy local: {}\nArithmetic local: {}\nUTC local: {}\nReturned local: {}"_el}.build(
            copy.isLocalTime(), later.isLocalTime(), utc.isLocalTime(), returned.isLocalTime()));

    // The same cached zone resolves each occurrence using its date, not today's offset.
    for (const auto season : {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), date}) {
        const auto occurrence = el::DateTime{season, el::Time{el::Hour{9}, el::Minute{}}, zone};
        el::io::printLine(
            el::StringFormat{"Local occurrence: {}\nOffset seconds: {}\nAbbreviation: {}"_el}.build(
                occurrence, occurrence.timeOffset().toSeconds().toRawValue(), occurrence.timeZoneAbbreviation()));
    }
}

}
