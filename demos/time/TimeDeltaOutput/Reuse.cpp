// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

#include <array>
#include <cstddef>

namespace demo {

/// Reuse a chain-configured presentation format and serialize with unchanged ELCL presets.
/// @notest{Compiled and executed documentation demo.}
void reuse() {
    const auto display = el::TimeDeltaFormat::longUnits()
                             .setUnitSeparator(", "_el)
                             .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                             .setShowFractions(true)
                             .setMaximumFractionDigits(2);
    const auto objectives = std::array{"Εξερεύνηση"_el, "Συνεργασία"_el};
    const auto intervals = std::array{el::TimeDelta::milliseconds(1250), el::TimeDelta::milliseconds(2650)};
    // Compare the output for each choice using the same input.
    for (std::size_t index = 0; index < objectives.size(); ++index) {
        el::io::printLine(el::StringFormat{"{}: {}"_el}.build(objectives[index], intervals[index].toString(display)));
    }
    const auto change = el::CalendarDelta{
        el::CalendarDeltaParts{.minutes = el::Minutes{4}, .months = el::Months{2}, .years = el::Years{1}}};
    el::io::printLine(el::StringFormat{"Configuration: {}"_el}.build(change.toString(el::TimeDeltaFormat::elcl())));
}

}
