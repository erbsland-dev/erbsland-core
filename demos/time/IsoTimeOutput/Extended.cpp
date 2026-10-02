// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Compare basic and extended ISO date and time fields.
/// @notest{Compiled and executed documentation demo.}
void extended() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto reading = el::DateTime{date, time, el::Seconds{5400}};
    auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::TimePrefix, el::IsoTimeFormat::TimeShift};
    el::io::printLine(
        el::StringFormat{"Basic date: {}; time: {}; date/time: {}"_el}.build(
            date.toIsoString(format), time.toIsoString(format), reading.toIsoString(format)));
    format.set(el::IsoTimeFormat::Extended);
    el::io::printLine(
        el::StringFormat{"Extended date: {}; time: {}; date/time: {}"_el}.build(
            date.toIsoString(format), time.toIsoString(format), reading.toIsoString(format)));
}

}
