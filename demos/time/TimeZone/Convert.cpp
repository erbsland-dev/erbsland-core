// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Display one UTC instant in different zones without changing its chronological position.
/// Local fields, resolved offsets, and abbreviations belong to the selected display zone.
/// @notest{Compiled and executed documentation demo.}
void convert() {
    const auto instant =
        el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 7, 15), el::Time{el::Hour{23}, el::Minute{30}}};
    for (
        const auto zone :
        {el::TimeZone::utc(),
            el::TimeZone{el::Hours{5}, el::Minutes{30}},
            el::TimeZone::fromNameOrThrow("Europe/Athens"_el),
            el::TimeZone::local()}) {
        const auto displayed = instant.toTimeZone(zone);
        el::io::printLine(
            el::StringFormat{"Local: {} {}\nUTC: {} {}\nOffset seconds: {}\nAbbreviation: {}"_el}.build(
                displayed.date(),
                displayed.time(),
                displayed.utcDate(),
                displayed.utcTime(),
                displayed.timeOffset().toSeconds().toRawValue(),
                displayed.timeZoneAbbreviation()));
        el::io::printLine(
            el::StringFormat{"Same ordering position: {}\nSame value: {}\nLocal origin: {}\nNamed zone: {}"_el}.build(
                (displayed <=> instant) == std::strong_ordering::equal,
                displayed == instant,
                displayed.timeZone().isLocalTime(),
                displayed.timeZone().isNamed()));
    }
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    for (
        const auto date :
        {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), el::Date::fromYearMonthDayOrThrow(2026, 7, 15)}) {
        const auto displayed = el::DateTime{date, el::Time{el::Hour{12}, el::Minute{}}}.toTimeZone(zone);
        el::io::printLine(
            el::StringFormat{"Seasonal display: {}\nOffset seconds: {}\nLabel: {}"_el}.build(
                displayed, displayed.timeOffset().toSeconds().toRawValue(), displayed.timeZoneAbbreviation()));
    }
}

}
