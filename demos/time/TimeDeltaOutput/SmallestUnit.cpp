// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <array>
#include <cstddef>

namespace demo {

/// Limit component output to each available fixed unit without changing the interval.
/// @notest{Compiled and executed documentation demo.}
void smallestUnit() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
        el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
    const auto units = std::array{
        el::TimeDeltaUnit::Nanoseconds,
        el::TimeDeltaUnit::Microseconds,
        el::TimeDeltaUnit::Milliseconds,
        el::TimeDeltaUnit::Seconds,
        el::TimeDeltaUnit::Minutes,
        el::TimeDeltaUnit::Hours,
        el::TimeDeltaUnit::Days,
        el::TimeDeltaUnit::Weeks};
    const auto labels = std::array{
        "Nanoseconds"_el,
        "Microseconds"_el,
        "Milliseconds"_el,
        "Seconds"_el,
        "Minutes"_el,
        "Hours"_el,
        "Days"_el,
        "Weeks"_el};
    // Compare the output for each choice using the same input.
    for (std::size_t index = 0; index < units.size(); ++index) {
        const auto format = el::TimeDeltaFormat{}.setSmallestUnit(units[index]);
        el::io::printLine(el::StringFormat{"{}: {}"_el}.build(labels[index], interval.toString(format)));
    }
}

}
