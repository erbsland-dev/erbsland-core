// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/all_time.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/TimeDeltaFormat.hpp>
#include <erbsland/TimeDeltaUnit.hpp>

namespace demo {

/// Enable fractional output at the chosen smallest unit with an explicit digit allowance.
/// @notest{Compiled and executed documentation demo.}
void fractions() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
        el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
    auto format = el::TimeDeltaFormat{}.setSmallestUnit(el::TimeDeltaUnit::Seconds).setMaximumFractionDigits(9);
    el::io::printLine(el::StringFormat{"Fractions off: {}"_el}.build(interval.toString(format)));
    format.setShowFractions(true);
    el::io::printLine(el::StringFormat{"Fractions on: {}"_el}.build(interval.toString(format)));
}

}
