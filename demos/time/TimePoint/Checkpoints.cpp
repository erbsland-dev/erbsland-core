// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimePointDemos.hpp"

#include <erbsland/time/TimePoint.hpp>

#include <chrono>

namespace demo {

/// Capture monotonic checkpoints explicitly and preserve steady-clock values on construction.
/// Default TimePoint construction selects the clock epoch, rather than the current time.
/// @notest{Compiled and executed documentation demo.}
void checkpoints() {
    const auto instrument = "Teleskop"_el;
    const auto epoch = el::TimePoint{};
    const auto checkpoint = el::TimePoint::now();
    const auto standardPoint = std::chrono::steady_clock::now();
    const auto imported = el::TimePoint{standardPoint};

    el::io::printLine(el::StringFormat{"{}: checkpoint recorded"_el}.build(instrument));
    el::io::printLine(
        el::StringFormat{"Default point is the steady-clock epoch: {}"_el}.build(
            epoch.toStdTimePoint() == std::chrono::steady_clock::time_point{}));
    el::io::printLine(
        el::StringFormat{"Imported point preserves its value: {}"_el}.build(
            imported.toStdTimePoint() == standardPoint));
    el::io::printLine(
        el::StringFormat{"Imported checkpoint is at least the earlier reading: {}"_el}.build(imported >= checkpoint));
}

}
