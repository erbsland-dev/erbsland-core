// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <array>
#include <cstddef>

namespace demo {

/// Render fixed and calendar intervals using default, long, and ELCL presets.
/// @notest{Compiled and executed documentation demo.}
void presets() {
    // Keep the interval exact; choose its presentation or conversion separately.
    const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
        el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
    const auto change = el::CalendarDelta{
        el::CalendarDeltaParts{.minutes = el::Minutes{4}, .months = el::Months{2}, .years = el::Years{1}}};
    const auto formats = std::array{
        el::TimeDeltaFormat{},
        el::TimeDeltaFormat::shortUnits(),
        el::TimeDeltaFormat::longUnits(),
        el::TimeDeltaFormat::elcl()};
    const auto labels = std::array{"Default"_el, "Short"_el, "Long"_el, "ELCL"_el};
    // Compare the output for each choice using the same input.
    for (std::size_t index = 0; index < formats.size(); ++index) {
        el::io::printLine(
            el::StringFormat{"{}; ELCL aliases: {}"_el}.build(labels[index], formats[index].usesElclUnitNames()));
        el::io::printLine(interval.toString(formats[index]));
        el::io::printLine(el::StringFormat{"Calendar: {}"_el}.build(change.toString(formats[index])));
    }
}

}
