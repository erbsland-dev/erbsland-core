// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <initializer_list>

namespace demo {

/// Choose short abbreviations or singular and plural English unit names.
/// @notest{Compiled and executed documentation demo.}
void unitStyle() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
        el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
    // Compare the output for each choice using the same input.
    for (const auto style : {el::TimeDeltaFormat::UnitStyle::Short, el::TimeDeltaFormat::UnitStyle::Long}) {
        const auto format = el::TimeDeltaFormat{}.setUnitStyle(style);
        el::io::printLine(interval.toString(format));
        el::io::printLine(
            el::StringFormat{"One: {}; two: {}; negative one: {}"_el}.build(
                el::TimeDelta::seconds(1).toString(format),
                el::TimeDelta::seconds(2).toString(format),
                el::TimeDelta::seconds(-1).toString(format)));
    }
    // Fractional unit values use the plural long name.
    const auto fractional = el::TimeDeltaFormat::longUnits()
                                .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                                .setShowFractions(true)
                                .setMaximumFractionDigits(3);
    el::io::printLine(el::TimeDelta::milliseconds(1250).toString(fractional));
}

}
