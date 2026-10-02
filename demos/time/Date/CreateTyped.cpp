// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Build observation dates from named calendar parts.
/// @notest{Compiled and executed documentation demo.}
void createTyped() {
    const auto observation = "Observation de la Lune"_el;
    const auto year = el::Year{2028};
    const auto month = el::Month::february();
    const auto day = el::Day{29};

    // Named parts make all three constructor orders unambiguous.
    const auto date = el::Date{year, month, day};
    const auto dayFirst = el::Date{day, month, year};
    const auto yearDayMonth = el::Date{year, day, month};
    const auto startOfYear = el::Date::fromParts(year);
    const auto startOfMonth = el::Date::fromParts(year, month);
    const auto checked = el::Date::fromPartsOrThrow(year, month, day);
    el::io::printLine(el::StringFormat{"{}: {}"_el}.build(observation, date));
    el::io::printLine(
        el::StringFormat{"Constructor orders agree: {}"_el}.build(date == dayFirst && date == yearDayMonth));
    el::io::printLine(
        el::StringFormat{"Year start: {}; month start: {}; checked: {}"_el}.build(startOfYear, startOfMonth, checked));

    // Typed parts clamp their input before the date checks the combination.
    const auto clampedMonth = el::Date{year, el::Month{13}, el::Day{1}};
    const auto impossible = el::Date{year, month, el::Day{31}};
    el::io::printLine(
        el::StringFormat{"Clamped month: {}; February 31 valid: {}"_el}.build(clampedMonth, impossible.isValid()));
}

}
