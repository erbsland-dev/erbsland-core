// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Observe saturation, guard zero divisors, and handle the asymmetric signed bounds.
/// @notest{Compiled and executed documentation demo.}
void boundaries() {
    const auto low = el::TimeDelta{el::Nanoseconds::minimum()};
    const auto high = el::TimeDelta{el::Nanoseconds::maximum()};
    const auto tick = el::TimeDelta::nanoseconds(1);
    el::io::printLine(
        el::StringFormat{"High + tick: {}; low - tick: {}; high * 2: {}"_el}.build(
            (high + tick).toNanoseconds().toRawValue(),
            (low - tick).toNanoseconds().toRawValue(),
            (high * el::TimeDelta::IntegerValue{2}).toNanoseconds().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Negated low: {}; absolute low: {}; low / -1: {}"_el}.build(
            (-low).toNanoseconds().toRawValue(),
            low.toAbsolute().toNanoseconds().toRawValue(),
            (low / el::TimeDelta::IntegerValue{-1}).toNanoseconds().toRawValue()));
    const auto divisor = el::TimeDelta::zero();
    if (divisor.isZero()) {
        el::io::printLine("Reject a zero divisor before calculating a ratio."_el);
    } else {
        el::io::printLine(el::StringFormat{"Ratio: {}"_el}.build((high / divisor).toRawValue()));
    }
    el::io::printLine(el::StringFormat{"Extreme ratio: {}"_el}.build((low / -tick).toRawValue()));
}

}
