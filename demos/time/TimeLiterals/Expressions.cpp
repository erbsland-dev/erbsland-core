// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeLiteralsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Pass typed literals to APIs and explicitly choose a common representation for mixed units.
/// @notest{Compiled and executed documentation demo.}
void expressions() {
    using namespace erbsland::time::literals;

    // A whole-second interval, a precise interval, and a calendar change have different meanings.
    const auto timeout = el::Duration{2_m};
    const auto latency = el::TimeDelta{250_ms};
    const auto calendar = el::CalendarDelta{1_h}.setMonths(el::Months{1});
    const auto shifted = el::Time{el::Hour{9}, el::Minute{0}}.addedWithWrap(el::Duration{30_m});
    el::io::printLine(
        el::StringFormat{"Timeout: {} s; latency: {}; calendar change: {}; time: {}"_el}.build(
            timeout.toSeconds().toRawValue(), latency.toString(), calendar.toString(), shifted.time.toString()));

    // Same-type arithmetic preserves the type. Convert before adding different amount types.
    const auto twoSamples = 250_ms + 250_ms;
    const auto commonUnit = (1_s).converted<el::Milliseconds>() + 250_ms;
    const auto preciseTotal = el::TimeDelta{1_s} + el::TimeDelta{250_ms};
    const auto mixedCalendar = el::CalendarDelta{el::Months{1}} + el::CalendarDelta{2_h};
    el::io::printLine(
        el::StringFormat{"Samples: {} ms; converted sum: {} ms; precise sum: {}; mixed change: {}"_el}.build(
            twoSamples.toRawValue(), commonUnit.toRawValue(), preciseTotal.toString(), mixedCalendar.toString()));
    // 1_s + 250_ms and Months{1} + 2_h have no amount operator: choose a representation first.
}

}
