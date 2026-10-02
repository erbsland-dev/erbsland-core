// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Present a fixed interval with default components or a chosen fractional unit.
/// @notest{Compiled and executed documentation demo.}
void display() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::nanoseconds(1'234'567'890);
    const auto format = el::TimeDeltaFormat{}
                            .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                            .setShowFractions(true)
                            .setMaximumFractionDigits(3);
    el::io::printLine(
        el::StringFormat{"Default: {}; customized: {}; unchanged ns: {}"_el}.build(
            interval.toString(), interval.toString(format), interval.toNanoseconds().toRawValue()));
}

}
