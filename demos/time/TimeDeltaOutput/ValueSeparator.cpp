// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <initializer_list>

namespace demo {

/// Customize separators while preserving the same interval components.
/// @notest{Compiled and executed documentation demo.}
void valueSeparator() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
        el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
    // Compare the output for each choice using the same input.
    for (const auto separator : {" "_el, ""_el, ":"_el}) {
        const auto format = el::TimeDeltaFormat{}.setValueSeparator(separator);
        el::io::printLine(interval.toString(format));
    }
}

}
