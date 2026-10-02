// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Replace the UTC designator with a numeric zero offset.
/// @notest{Compiled and executed documentation demo.}
void completeOffset() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto utc = el::DateTime{date, time};
    auto format = el::IsoTimeFormatFlags{
        el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix, el::IsoTimeFormat::TimeShift};
    el::io::printLine(el::StringFormat{"UTC designator: {}"_el}.build(utc.toIsoString(format)));
    format.set(el::IsoTimeFormat::TimeShiftAlwaysComplete);
    el::io::printLine(el::StringFormat{"Numeric UTC offset: {}"_el}.build(utc.toIsoString(format)));
    format.clear(el::IsoTimeFormat::Extended);
    el::io::printLine(el::StringFormat{"Basic numeric offset: {}"_el}.build(utc.toIsoString(format)));
    format.clear(el::IsoTimeFormat::TimeShift);
    el::io::printLine(el::StringFormat{"Without TimeShift: {}"_el}.build(utc.toIsoString(format)));
}

}
