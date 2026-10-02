// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Keep a transient zone identifier within its database context and inspect low-level offset values.
/// Persist the named zone's primary name rather than its numeric identifier or abbreviation index.
/// @notest{Compiled and executed documentation demo.}
void details() {
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto restored = el::TimeZone{zone.id()};
    const auto utcId = el::TimeZoneId{};
    const auto copiedId = el::TimeZoneId{zone.id().toRawValue()};
    el::io::printLine(
        el::StringFormat{"Stored name: {}\nId round trip: {}\nCopied id: {}\nDefault id UTC: {}"_el}.build(
            zone.name(), restored == zone, copiedId == zone.id(), utcId.isUtc()));

    // Public UTC and fixed constructors do not require database metadata.
    const auto utc = el::time::tz::TimeOffset{};
    const auto fixed = el::time::tz::TimeOffset{el::Seconds{19800}, true};
    for (const auto offset : {utc, fixed}) {
        el::io::printLine(
            el::StringFormat{
                "Seconds: {}\nUTC: {}\nFixed: {}\nNamed: {}\nDST: {}\nLocal: {}\nUTC id: {}\nAbbreviation index: {}"_el}
                .build(
                    offset.offset().toRawValue(),
                    offset.isUtc(),
                    offset.isStaticOffset(),
                    offset.isZone(),
                    offset.isDst(),
                    offset.isLocalTime(),
                    offset.zoneId().isUtc(),
                    offset.abbreviationId()));
    }
    const auto occurrence = el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 7, 15), el::Time{}, zone};
    el::io::printLine(
        el::StringFormat{"Application-level resolved offset seconds: {}\nDisplay abbreviation: {}"_el}.build(
            occurrence.timeOffset().toSeconds().toRawValue(), occurrence.timeZoneAbbreviation()));
}

}
