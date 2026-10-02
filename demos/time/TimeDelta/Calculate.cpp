// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Combine intervals, scale a budget, and distinguish interval division from ratios.
/// @notest{Compiled and executed documentation demo.}
void calculate() {
    using namespace el::time::literals;

    const auto turn = el::TimeDelta{1250_ms};
    const auto setup = el::TimeDelta{250_ms};
    const auto budget = (turn + setup) * el::TimeDelta::IntegerValue{3};
    auto remaining = budget;
    remaining -= turn;
    remaining += setup;
    remaining *= el::TimeDelta::IntegerValue{2};
    remaining /= el::TimeDelta::IntegerValue{3};
    el::io::printLine(
        el::StringFormat{"Budget: {}; remaining: {}; setup fits: {}"_el}.build(
            budget.toString(), remaining.toString(), setup < remaining));
    el::io::printLine(
        el::StringFormat{"Per player: {}; complete turns: {}"_el}.build(
            (budget / el::TimeDelta::IntegerValue{2}).toString(), (budget / turn).toRawValue()));

    const auto correction = -setup;
    el::io::printLine(
        el::StringFormat{"Negative: {}; positive: {}; zero difference: {}; absolute: {}"_el}.build(
            correction.isNegative(), turn.isPositive(), (turn - turn).isZero(), correction.toAbsolute().toString()));
    // minimum imposes a floor; it does not cap a value.
    el::io::printLine(
        el::StringFormat{"Minimum delay: {}; negative integer division: {} ns"_el}.build(
            setup.minimum(1_s).toString(),
            (el::TimeDelta::nanoseconds(-5) / el::TimeDelta::IntegerValue{2}).toNanoseconds().toRawValue()));
}

}
