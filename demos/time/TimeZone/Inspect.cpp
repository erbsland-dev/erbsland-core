// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Inspect a zone's category separately from its system-local origin and resolved offset.
/// Zone equality compares the stored identity and origin, rather than a seasonal offset.
/// @notest{Compiled and executed documentation demo.}
void inspect() {
    const auto named = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto fixed = el::TimeZone{el::Hours{2}};
    for (const auto zone : {el::TimeZone::utc(), fixed, named, el::TimeZone::local()}) {
        el::io::printLine(
            el::StringFormat{"UTC: {}\nFixed: {}\nNamed: {}\nLocal: {}\nName: {}\nUTC id: {}\nStatic seconds: {}"_el}
                .build(
                    zone.isUtc(),
                    zone.isStaticOffset(),
                    zone.isNamed(),
                    zone.isLocalTime(),
                    zone.name(),
                    zone.id().isUtc(),
                    zone.staticOffset().toSeconds().toRawValue()));
    }
    el::io::printLine(
        el::StringFormat{"Named equals fixed: {}\nAlias equals primary: {}"_el}.build(
            named == fixed,
            el::TimeZone::fromNameOrThrow("US/Eastern"_el) == el::TimeZone::fromNameOrThrow("America/New_York"_el)));
    for (
        const auto date :
        {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), el::Date::fromYearMonthDayOrThrow(2026, 7, 15)}) {
        const auto namedValue = el::DateTime{date, el::Time{}, named};
        const auto fixedValue = el::DateTime{date, el::Time{}, fixed};
        el::io::printLine(
            el::StringFormat{"{}: named seconds {}\nFixed seconds {}\nOffsets equal: {}"_el}.build(
                date,
                namedValue.timeOffset().toSeconds().toRawValue(),
                fixedValue.timeOffset().toSeconds().toRawValue(),
                namedValue.timeOffset() == fixedValue.timeOffset()));
    }
}

}
