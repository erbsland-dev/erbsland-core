// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateDemos.hpp"

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/time/all.hpp>

namespace demo {

/// Validate supplied dates and choose calendar boundaries.
/// @notest{Compiled and executed documentation demo.}
void createChecked() {
    // Validate raw fields before they become clamped calendar parts.
    const auto supplied = el::Date::fromYearMonthDay(2028, 2, 29);
    const auto rejected = el::Date::fromYearMonthDay(2028, 13, 1);
    el::io::printLine(el::StringFormat{"Supplied date: {}; month 13 valid: {}"_el}.build(supplied, rejected.isValid()));
    try {
        const auto invalid = el::Date::fromYearMonthDayOrThrow(2027, 2, 29);
        el::io::printLine(el::StringFormat{"Accepted: {}"_el}.build(invalid));
    } catch (const el::OutOfRangeError &) {
        el::io::printLine("Rejected February 29 in a common year."_el);
    }
    try {
        const auto invalid = el::Date::fromPartsOrThrow(el::Year{2028}, el::Month::february(), el::Day{31});
        el::io::printLine(el::StringFormat{"Accepted: {}"_el}.build(invalid));
    } catch (const el::OutOfRangeError &) {
        el::io::printLine("Rejected February 31 from typed parts."_el);
    }

    const auto restored = el::Date::fromDaysSinceEpoch(supplied.toDaysSinceEpoch());
    const auto beforeEpoch = el::Date::fromDaysSinceEpoch(el::Days{-1});
    el::io::printLine(
        el::StringFormat{"Restored: {}; negative day count valid: {}"_el}.build(restored, beforeEpoch.isValid()));
    el::io::printLine(
        el::StringFormat{"Epoch: {}; first: {}; last: {}"_el}.build(
            el::Date::epoch(), el::Date::first(), el::Date::last()));
    const auto year = el::Year{2028};
    const auto month = el::Month::february();
    el::io::printLine(
        el::StringFormat{"Year boundaries: {} to {}"_el}.build(el::Date::firstDay(year), el::Date::lastDay(year)));
    el::io::printLine(
        el::StringFormat{"Month boundaries: {} to {}"_el}.build(
            el::Date::firstDay(year, month), el::Date::lastDay(year, month)));
}

}
