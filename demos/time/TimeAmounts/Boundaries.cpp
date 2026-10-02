// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeAmountsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Check saturating addition, subtraction, multiplication, and minimum-value negation.
/// @notest{Compiled and executed documentation demo.}
void boundaries() {
    const auto maximum = el::Seconds::maximum();
    const auto minimum = el::Seconds::minimum();
    const auto one = el::Seconds{1};
    el::io::printLine(
        el::StringFormat{"Add would saturate: {}; subtract would saturate: {}; multiply would saturate: {}"_el}.build(
            maximum.wouldAddSaturate(one), minimum.wouldSubtractSaturate(one), maximum.wouldMultiplySaturate(2)));
    el::io::printLine(
        el::StringFormat{"Maximum plus one: {}; minimum minus one: {}; maximum times two: {}"_el}.build(
            (maximum + one).toRawValue(), (minimum - one).toRawValue(), (maximum * 2).toRawValue()));
    el::io::printLine(
        el::StringFormat{"Minimum: {}; negated minimum: {}; saturated negation: {}"_el}.build(
            minimum.toRawValue(), (-minimum).toRawValue(), (-minimum).isMaximum()));
}

}
