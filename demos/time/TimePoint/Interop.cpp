// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimePointDemos.hpp"

#include <erbsland/time/Literals.hpp>
#include <erbsland/time/TimePoint.hpp>

#include <thread>

namespace demo {

/// Pass a monotonic deadline to a standard-library API in the same steady-clock domain.
/// toStdTimePoint() and the TimePoint constructor preserve a steady-clock reading.
/// sleep_until() may return later than its requested deadline because of scheduling.
/// @notest{Compiled and executed documentation demo.}
void interop() {
    using namespace el::time::literals;
    const auto instrument = "Teleskop"_el;
    const auto start = el::TimePoint::now();
    const auto captureDeadline = start + el::TimeDelta{2_ms};
    const auto standardDeadline = captureDeadline.toStdTimePoint();
    const auto importedDeadline = el::TimePoint{standardDeadline};

    // The standard API receives a steady-clock point, retaining the original clock domain.
    std::this_thread::sleep_until(standardDeadline);
    const auto finished = el::TimePoint::now();
    el::io::printLine(
        el::StringFormat{"{}: deadline round trip preserved: {}"_el}.build(
            instrument, importedDeadline == captureDeadline));
    el::io::printLine(
        el::StringFormat{"Elapsed after waiting: {}; deadline reached: {}"_el}.build(
            (finished - start).toString(), finished >= captureDeadline));
}

}
