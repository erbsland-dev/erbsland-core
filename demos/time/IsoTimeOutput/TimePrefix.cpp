// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Select the T prefix and combined date/time separator.
/// @notest{Compiled and executed documentation demo.}
void timePrefix() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto reading = el::DateTime{date, time, el::Seconds{5400}};
    auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended};
    el::io::printLine(
        el::StringFormat{"Without prefix: {}; {}"_el}.build(time.toIsoString(format), reading.toIsoString(format)));
    format.set(el::IsoTimeFormat::TimePrefix);
    el::io::printLine(
        el::StringFormat{"With prefix: {}; {}"_el}.build(time.toIsoString(format), reading.toIsoString(format)));
    el::io::printLine(el::StringFormat{"Date stays: {}"_el}.build(date.toIsoString(format)));
}

}
