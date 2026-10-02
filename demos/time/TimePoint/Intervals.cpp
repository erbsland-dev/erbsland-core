// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimePointDemos.hpp"

#include <erbsland/time/Literals.hpp>
#include <erbsland/time/TimePoint.hpp>

namespace demo {

/// Calculate signed distances between monotonic points and from a point to now.
/// end - start and start.timeDeltaTo(end) have the same direction.
/// Reversing the endpoints negates the interval.
/// @notest{Compiled and executed documentation demo.}
void intervals() {
    using namespace el::time::literals;
    const auto checkpoint = el::TimePoint::now();

    // A planned offset gives exact examples without waiting for a particular elapsed reading.
    const auto nextCapture = checkpoint + el::TimeDelta{25_ms};
    const auto forward = nextCapture - checkpoint;
    const auto backward = nextCapture.timeDeltaTo(checkpoint);
    el::io::printLine(el::StringFormat{"Forward: {}; reverse: {}"_el}.build(forward.toString(), backward.toString()));
    el::io::printLine(
        el::StringFormat{"Subtraction and timeDeltaTo agree: {}; points ordered: {}"_el}.build(
            forward == checkpoint.timeDeltaTo(nextCapture), checkpoint < nextCapture));
    el::io::printLine(
        el::StringFormat{"Planned interval in whole milliseconds: {}"_el}.build(forward.toMilliseconds().toRawValue()));

    // This is a real clock query, so its result varies between runs.
    const auto elapsed = checkpoint.timeDeltaToNow();
    el::io::printLine(el::StringFormat{"Elapsed since the checkpoint: {}"_el}.build(elapsed.toString()));
}

}
