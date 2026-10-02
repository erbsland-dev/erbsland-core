// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DurationDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Combine intervals, compare budgets, and observe saturation at second bounds.
/// @notest{Compiled and executed documentation demo.}
void calculate() {
    const auto warmup = el::Duration{el::Minutes{2}};
    const auto measurement = el::Duration{el::Seconds{45}};
    const auto budget = el::Duration{el::Minutes{3}};
    const auto required = warmup + measurement;
    const auto remaining = budget - required;
    el::io::printLine(
        el::StringFormat{"Required: {} s; remaining: {} s; within budget: {}"_el}.build(
            required.toSeconds().toRawValue(), remaining.toSeconds().toRawValue(), required <= budget));
    auto revised = required;
    revised += el::Duration{el::Seconds{10}};
    revised -= el::Duration{el::Seconds{5}};
    el::io::printLine(
        el::StringFormat{"Revised: {} s; equals original: {}; positive: {}; negative: {}; zero: {}"_el}.build(
            revised.toSeconds().toRawValue(),
            revised == required,
            revised.isPositive(),
            (-revised).isNegative(),
            (revised - revised).isZero()));

    // Arithmetic clamps rather than wrapping or throwing at a representable bound.
    const auto maximum = el::Duration{el::Seconds::maximum()};
    const auto minimum = el::Duration{el::Seconds::minimum()};
    const auto one = el::Duration{el::Seconds{1}};
    el::io::printLine(
        el::StringFormat{"Adding one to maximum would saturate: {}"_el}.build(
            maximum.toSeconds().wouldAddSaturate(one.toSeconds())));
    el::io::printLine(el::StringFormat{"Maximum plus one: {}"_el}.build((maximum + one).toSeconds().toRawValue()));
    el::io::printLine(el::StringFormat{"Minimum minus one: {}"_el}.build((minimum - one).toSeconds().toRawValue()));
    el::io::printLine(el::StringFormat{"Negated minimum: {}"_el}.build((-minimum).toSeconds().toRawValue()));
}

}
