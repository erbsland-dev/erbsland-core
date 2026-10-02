// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <array>
#include <utility>

namespace demo {

/// Compare every ISO output precision on the same stored values.
/// @notest{Compiled and executed documentation demo.}
void precision() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto reading = el::DateTime{date, time, el::Seconds{5400}};
    const auto format = el::IsoTimeFormatFlags{
        el::IsoTimeFormat::Extended,
        el::IsoTimeFormat::TimePrefix,
        el::IsoTimeFormat::TimeShift,
        el::IsoTimeFormat::UseDotFraction};
    const auto levels = std::array{
        std::pair{"Year"_el, el::DateTimePrecision::Year},
        std::pair{"Month"_el, el::DateTimePrecision::Month},
        std::pair{"Day"_el, el::DateTimePrecision::Day},
        std::pair{"Hour"_el, el::DateTimePrecision::Hour},
        std::pair{"Minute"_el, el::DateTimePrecision::Minute},
        std::pair{"Second"_el, el::DateTimePrecision::Second},
        std::pair{"Millisecond"_el, el::DateTimePrecision::Millisecond},
        std::pair{"Microsecond"_el, el::DateTimePrecision::Microsecond},
        std::pair{"Nanosecond"_el, el::DateTimePrecision::Nanosecond}};
    for (const auto &[name, precision] : levels) {
        el::io::printLine(el::StringFormat{"{}:"_el}.build(name));
        el::io::printLine(el::StringFormat{"  Date: {}"_el}.build(date.toIsoString(format, precision)));
        el::io::printLine(el::StringFormat{"  Time: {}"_el}.build(time.toIsoString(format, precision)));
        el::io::printLine(el::StringFormat{"  Date/time: {}"_el}.build(reading.toIsoString(format, precision)));
    }
}

}
