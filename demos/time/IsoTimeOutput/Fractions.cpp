// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IsoTimeOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Choose a fraction separator and retain fixed-width fractional digits.
/// @notest{Compiled and executed documentation demo.}
void fractions() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
    const auto reading = el::DateTime{date, time, el::Seconds{5400}};
    auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix};
    const auto precision = el::DateTimePrecision::Millisecond;
    el::io::printLine(
        el::StringFormat{"Comma: {}; {}"_el}.build(
            time.toIsoString(format, precision), reading.toIsoString(format, precision)));
    format.set(el::IsoTimeFormat::UseDotFraction);
    el::io::printLine(
        el::StringFormat{"Dot: {}; {}"_el}.build(
            time.toIsoString(format, precision), reading.toIsoString(format, precision)));
    const auto exact = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}};
    const auto trailing = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{120000000}};
    el::io::printLine(
        el::StringFormat{"Zero fraction: {}; trailing zeros: {}"_el}.build(
            exact.toIsoString(format, precision), trailing.toIsoString(format, precision)));
    el::io::printLine(
        el::StringFormat{"Second precision: {}"_el}.build(time.toIsoString(format, el::DateTimePrecision::Second)));
}

}
