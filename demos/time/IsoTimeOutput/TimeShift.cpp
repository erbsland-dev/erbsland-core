// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Include a resolved UTC offset without changing the display zone.
/// @notest{Compiled and executed documentation demo.}
void timeShift() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto reading = el::DateTime{date, time, el::Seconds{5400}};
    auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix};
    el::io::printLine(el::StringFormat{"Offset omitted: {}"_el}.build(reading.toIsoString(format)));
    format.set(el::IsoTimeFormat::TimeShift);
    el::io::printLine(el::StringFormat{"Offset included: {}"_el}.build(reading.toIsoString(format)));
    const auto utc = el::DateTime{reading.utcDate(), reading.utcTime()};
    el::io::printLine(el::StringFormat{"Same instant in UTC: {}"_el}.build(utc.toIsoString(format)));
    el::io::printLine(
        el::StringFormat{"Standalone fields: {}; {}"_el}.build(date.toIsoString(format), time.toIsoString(format)));
}

}
