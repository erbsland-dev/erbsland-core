// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ElapsedTimerDemos.hpp"

#include <erbsland/time/ElapsedTimer.hpp>
#include <erbsland/time/TimeDeltaFormat.hpp>

#include <array>

namespace demo {

/// Measure an operation with a monotonic timer and read cumulative elapsed intervals.
/// ElapsedTimer starts on construction; elapsed() does not restart it.
/// TimeDelta preserves the measured interval for conversion and formatting.
/// @notest{Compiled and executed documentation demo.}
void measure() {
    const auto detector = "Algılayıcı"_el;
    const auto samples = std::array{12.0, 14.0, 11.0, 15.0, 13.0, 16.0};
    const auto timer = el::ElapsedTimer{};

    // Calculate a mean, then read elapsed time twice from the same starting point.
    auto total = 0.0;
    for (const auto sample : samples) {
        total += sample;
    }
    const auto firstReading = timer.elapsed();
    const auto mean = total / samples.size();
    const auto secondReading = timer.elapsed();

    // Whole milliseconds may be zero for a short operation; retain the precise interval.
    const auto format = el::TimeDeltaFormat::longUnits().setUnitSeparator(", "_el);
    el::io::printLine(el::StringFormat{"{}: mean {}"_el}.build(detector, mean));
    el::io::printLine(
        el::StringFormat{"First reading: {}; second reading: {}"_el}.build(
            firstReading.toString(), secondReading.toString()));
    el::io::printLine(
        el::StringFormat{"Whole milliseconds: {}; formatted: {}"_el}.build(
            secondReading.toMilliseconds().toRawValue(), secondReading.toString(format)));
    el::io::printLine(
        el::StringFormat{"Second reading is at least the first: {}"_el}.build(secondReading >= firstReading));
}

}
