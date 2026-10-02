// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Preserve second-level offsets and understand complete offset output.
/// @notest{Compiled and executed documentation demo.}
void offsetSeconds() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto reading = el::DateTime{date, time, el::Seconds{5400}};
    const auto secondOffset = el::DateTime{date, time, el::Seconds{5417}};
    auto format = el::IsoTimeFormatFlags{
        el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix, el::IsoTimeFormat::TimeShift};
    el::io::printLine(el::StringFormat{"Offset seconds omitted: {}"_el}.build(secondOffset.toIsoString(format)));
    format.set(el::IsoTimeFormat::TimeShiftUpToSeconds);
    el::io::printLine(el::StringFormat{"Offset seconds retained: {}"_el}.build(secondOffset.toIsoString(format)));
    el::io::printLine(el::StringFormat{"Whole-minute offset: {}"_el}.build(reading.toIsoString(format)));
    format.set(el::IsoTimeFormat::TimeShiftAlwaysComplete);
    el::io::printLine(el::StringFormat{"Complete whole-minute offset: {}"_el}.build(reading.toIsoString(format)));
    const auto utc = el::DateTime{date, time};
    el::io::printLine(el::StringFormat{"Complete UTC offset: {}"_el}.build(utc.toIsoString(format)));
    const auto all = el::IsoTimeFormatFlags{el::IsoTimeFormat::All};
    el::io::printLine(
        el::StringFormat{"All flags: {}"_el}.build(secondOffset.toIsoString(all, el::DateTimePrecision::Nanosecond)));
}

}
