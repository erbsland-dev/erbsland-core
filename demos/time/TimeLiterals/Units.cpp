// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeLiteralsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Integer suffixes create typed amounts, including zero and negated values.
/// @notest{Compiled and executed documentation demo.}
void units() {
    using namespace erbsland::time::literals;

    const el::Nanoseconds tick = 1_ns;
    const el::Microseconds pulse = 20_us;
    const el::Milliseconds sample = 250_ms;
    const el::Seconds pause = 3_s;
    const el::Minutes observation = 2_m;
    const el::Hours window = 1_h;
    el::io::printLine(
        el::StringFormat{"{} ns; {} us; {} ms; {} s; {} min; {} h"_el}.build(
            tick.toRawValue(),
            pulse.toRawValue(),
            sample.toRawValue(),
            pause.toRawValue(),
            observation.toRawValue(),
            window.toRawValue()));

    // Digit separators change spelling, not the amount or its unit.
    const auto detailedSample = 1'500_ms;
    const auto correction = -250_ms;
    const auto noDelay = 0_s;
    el::io::printLine(
        el::StringFormat{"Sample: {} ms; correction: {} ms; zero: {}"_el}.build(
            detailedSample.toRawValue(), correction.toRawValue(), noDelay.isZero()));
    const auto season = el::Months{3};
    const auto fixedDays = el::Days{7};
    el::io::printLine(
        el::StringFormat{"Explicit amounts: {} months; {} fixed days"_el}.build(
            season.toRawValue(), fixedDays.toRawValue()));
}

}
