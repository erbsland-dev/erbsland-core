// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Interpret local calendar fields in a zone, or convert an already-known UTC instant to that zone.
/// These operations start with different information and need not select the same instant.
/// @notest{Compiled and executed documentation demo.}
void resolve() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 15);
    const auto reading = el::Time{el::Hour{9}, el::Minute{15}};
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto localInput = el::DateTime{date, reading, zone};
    const auto utcInput = el::DateTime{date, reading};
    const auto converted = utcInput.toTimeZone(zone);
    el::io::printLine(el::StringFormat{"Local input: {}\nIts UTC: {}"_el}.build(localInput, localInput.toUtc()));
    el::io::printLine(
        el::StringFormat{"UTC input: {}\nIts zoned display: {}\nSame instant as local input: {}"_el}.build(
            utcInput, converted, converted.toUtc() == localInput.toUtc()));
    el::io::printLine(
        el::StringFormat{"Invalid local date accepted: {}"_el}.build(
            el::DateTime{el::Date{}, reading, zone}.isValid()));
}

}
