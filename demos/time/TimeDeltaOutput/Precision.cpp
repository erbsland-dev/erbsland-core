// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <array>

namespace demo {

/// Combine precision settings and inspect truncation, zeros, signs, and trailing zeros.
/// @notest{Compiled and executed documentation demo.}
void precision() {
    auto seconds = el::TimeDeltaFormat{}
                       .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                       .setShowFractions(true)
                       .setMaximumFractionDigits(3);
    const auto values = std::array{
        el::TimeDelta::zero(),
        el::TimeDelta::milliseconds(1250),
        el::TimeDelta::milliseconds(-1250),
        el::TimeDelta::nanoseconds(1'999'999'999),
        el::TimeDelta::microseconds(1),
        el::TimeDelta::microseconds(-1)};
    // Compare the output for each choice using the same input.
    for (const auto value : values) {
        el::io::printLine(
            el::StringFormat{"{} ns: {}"_el}.build(value.toNanoseconds().toRawValue(), value.toString(seconds)));
    }
    seconds.setShowFractions(false);
    el::io::printLine(el::StringFormat{"Hidden sub-second input: {}"_el}.build(values.back().toString(seconds)));
    const auto minutes = el::TimeDeltaFormat{}
                             .setSmallestUnit(el::TimeDeltaUnit::Minutes)
                             .setShowFractions(true)
                             .setMaximumFractionDigits(3);
    el::io::printLine(
        el::StringFormat{"One second as minutes: {}; zero as minutes: {}"_el}.build(
            el::TimeDelta::seconds(1).toString(minutes), el::TimeDelta::zero().toString(minutes)));
}

}
