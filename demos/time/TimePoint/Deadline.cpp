// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimePointDemos.hpp"

#include <erbsland/time/Literals.hpp>
#include <erbsland/time/TimePoint.hpp>

namespace demo {

/// Represent and adjust a monotonic deadline without waiting or scheduling work.
/// Subtract a captured current point to obtain a signed remaining interval.
/// A nonpositive interval means the deadline has been reached or passed.
/// @notest{Compiled and executed documentation demo.}
void deadline() {
    using namespace el::time::literals;
    const auto instrument = "Teleskop"_el;
    auto captureDeadline = el::TimePoint::inFuture(el::TimeDelta{25_ms});

    // Allow extra preparation time, then reserve a small interval before the capture.
    captureDeadline += el::TimeDelta{5_ms};
    captureDeadline -= el::TimeDelta{1_ms};
    const auto preparationDeadline = captureDeadline - el::TimeDelta{2_ms};
    const auto extendedDeadline = captureDeadline + el::TimeDelta{10_ms};

    // Capture now once so the comparison and remaining interval use the same reading.
    const auto now = el::TimePoint::now();
    const auto remaining = now.timeDeltaTo(captureDeadline);
    const auto expiredDeadline = now - el::TimeDelta{2_ms};
    el::io::printLine(
        el::StringFormat{"{}: deadline reached: {}; remaining: {}"_el}.build(
            instrument, now >= captureDeadline, remaining.toString()));
    el::io::printLine(
        el::StringFormat{"Preparation before capture: {}; extension after capture: {}"_el}.build(
            (preparationDeadline < captureDeadline), (extendedDeadline > captureDeadline)));
    el::io::printLine(
        el::StringFormat{"Expired deadline reached: {}; remaining: {}"_el}.build(
            now >= expiredDeadline, (expiredDeadline - now).toString()));
}

}
