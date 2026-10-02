// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Convert observation dates to text and epoch day counts.
/// @notest{Compiled and executed documentation demo.}
void convert() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    el::io::printLine(el::StringFormat{"Display: {}; default ISO: {}"_el}.build(date.toString(), date.toIsoString()));
    el::io::printLine(el::StringFormat{"Days since Core epoch: {}"_el}.build(date.toDaysSinceEpoch().toRawValue()));
    el::io::printLine(el::StringFormat{"Compact ISO: {}"_el}.build(date.toIsoString(el::IsoTimeFormatFlags{})));
    el::io::printLine(
        el::StringFormat{"Month precision: {}"_el}.build(
            date.toIsoString(el::cDefaultDateFormat, el::DateTimePrecision::Month)));
    const auto invalid = el::Date{};
    el::io::printLine(
        el::StringFormat{"Invalid display: '{}'; invalid day count: {}"_el}.build(
            invalid.toString(), invalid.toDaysSinceEpoch().toRawValue()));
}

}
