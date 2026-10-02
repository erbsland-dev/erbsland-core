// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <cstdint>
#include <initializer_list>

namespace demo {

/// Truncate fractional digits and observe the nine-digit upper limit.
/// @notest{Compiled and executed documentation demo.}
void fractionDigits() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
        el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
    // Compare the output for each choice using the same input.
    for (const auto requested : {0, 2, 3, 6, 9, 12}) {
        const auto format = el::TimeDeltaFormat{}
                                .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                                .setShowFractions(true)
                                .setMaximumFractionDigits(static_cast<uint8_t>(requested));
        el::io::printLine(
            el::StringFormat{"Requested {}; effective {}: {}"_el}.build(
                requested, format.maximumFractionDigits(), interval.toString(format)));
    }
}

}
