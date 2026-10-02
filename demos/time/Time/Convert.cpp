// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Preserve or discard fractional precision when converting times.
/// @notest{Compiled and executed documentation demo.}
void convert() {
    const auto reading = el::Time{el::Hour{23}, el::Minute{45}, el::Second{12}, el::Nanoseconds{123456789}};
    el::io::printLine(
        el::StringFormat{"Whole seconds since midnight: {}"_el}.build(reading.toSecondsSinceMidnight().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Nanoseconds since midnight: {}"_el}.build(reading.toNanosecondsSinceMidnight().toRawValue()));
    const auto whole = reading.durationSinceMidnight();
    const auto precise = reading.timeDeltaSinceMidnight();
    el::io::printLine(
        el::StringFormat{"Duration round trip: {}; TimeDelta round trip: {}"_el}.build(
            el::Time::fromDurationSinceMidnight(whole), el::Time::fromDurationSinceMidnight(precise)));
    el::io::printLine(
        el::StringFormat{"Display: {}; default ISO: {}"_el}.build(reading.toString(), reading.toIsoString()));
    const auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::UseDotFraction};
    el::io::printLine(
        el::StringFormat{"Nanosecond ISO: {}"_el}.build(
            reading.toIsoString(format, el::DateTimePrecision::Nanosecond)));
    el::io::printLine(
        el::StringFormat{"Millisecond ISO: {}"_el}.build(
            reading.toIsoString(format, el::DateTimePrecision::Millisecond)));
}

}
