// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Choose ISO output for a recorded date and clock reading.
/// @notest{Compiled and executed documentation demo.}
void defaults() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto reading = el::DateTime{date, time, el::Seconds{5400}};
    const auto prototype = "試作センサー"_el;
    el::io::printLine(el::StringFormat{"{}: {}"_el}.build(prototype, reading.toIsoString()));
    el::io::printLine(el::StringFormat{"Date: {}; time: {}"_el}.build(date.toIsoString(), time.toIsoString()));

    // Explicit flags and precision retain the fractional reading and its offset.
    const auto format = el::IsoTimeFormatFlags{
        el::IsoTimeFormat::Extended,
        el::IsoTimeFormat::TimePrefix,
        el::IsoTimeFormat::TimeShift,
        el::IsoTimeFormat::UseDotFraction};
    el::io::printLine(
        el::StringFormat{"Exchange text: {}"_el}.build(reading.toIsoString(format, el::DateTimePrecision::Nanosecond)));
    el::io::printLine(el::StringFormat{"Readable display: {}"_el}.build(reading.toString()));
    el::io::printLine(
        el::StringFormat{"Invalid date: [{}]; invalid date/time: [{}]"_el}.build(
            el::Date{}.toIsoString(), el::DateTime{}.toIsoString()));
}

}
