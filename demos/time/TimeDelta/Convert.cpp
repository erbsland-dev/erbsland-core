// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

#include <initializer_list>

namespace demo {

/// Convert total interval amounts and preserve or deliberately discard fractional precision.
/// @notest{Compiled and executed documentation demo.}
void convert() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::nanoseconds(1'234'567'890);
    el::io::printLine(
        el::StringFormat{"Totals: {} ns; {} ms; {} s"_el}.build(
            interval.toNanoseconds().toRawValue(),
            interval.toMilliseconds().toRawValue(),
            interval.toSeconds().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Approximate seconds: {}; approximate days: {}"_el}.build(
            interval.toSecondsWithFractions(), interval.toDaysWithFractions()));
    el::io::printLine(el::StringFormat{"Chrono ns: {}"_el}.build(interval.toStdNanoseconds().count()));
    // Compare the output for each choice using the same input.
    for (const auto input : {-1500, -999, 999, 1500}) {
        const auto value = el::TimeDelta::milliseconds(input);
        el::io::printLine(
            el::StringFormat{"{} ms: {} seconds; Duration {} seconds"_el}.build(
                input, value.toSeconds().toRawValue(), value.toDuration().toSeconds().toRawValue()));
    }
}

}
