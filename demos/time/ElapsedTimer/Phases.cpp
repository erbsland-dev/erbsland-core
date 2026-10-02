// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ElapsedTimerDemos.hpp"

#include <erbsland/time/ElapsedTimer.hpp>

#include <array>

namespace demo {

/// Restart a timer between phases and copy its starting point for a total measurement.
/// Save elapsed() before restart(); restart() returns no previous interval.
/// A restart changes only the timer receiving the call, not its copies.
/// @notest{Compiled and executed documentation demo.}
void phases() {
    const auto sensor = "Işık sensörü"_el;
    auto samples = std::array{12.0, 14.0, 11.0, 15.0, 13.0, 16.0};
    auto phaseTimer = el::ElapsedTimer{};
    const auto totalTimer = phaseTimer;

    // First phase: remove the detector's baseline from every sample.
    for (auto &sample : samples) {
        sample -= 2.0;
    }
    const auto calibrationElapsed = phaseTimer.elapsed();
    phaseTimer.restart();

    // Second phase: combine the calibrated readings.
    auto total = 0.0;
    for (const auto sample : samples) {
        total += sample;
    }
    const auto reductionElapsed = phaseTimer.elapsed();
    const auto totalElapsed = totalTimer.elapsed();

    el::io::printLine(el::StringFormat{"{}: calibrated total {}"_el}.build(sensor, total));
    el::io::printLine(
        el::StringFormat{"Calibration: {}; reduction: {}; total: {}"_el}.build(
            calibrationElapsed.toString(), reductionElapsed.toString(), totalElapsed.toString()));
    el::io::printLine(
        el::StringFormat{"Copied timer still includes the first phase: {}"_el}.build(
            totalElapsed >= calibrationElapsed + reductionElapsed));
}

}
