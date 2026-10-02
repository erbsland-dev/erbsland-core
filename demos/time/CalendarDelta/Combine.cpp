// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Combine changes by component without normalizing or imposing an ordering.
/// @notest{Compiled and executed documentation demo.}
void combine() {
    const auto monthly = el::CalendarDelta{el::Months{1}};
    const auto extra = el::CalendarDelta{el::Days{2}};
    const auto combined = monthly + extra;
    auto edited = combined;
    edited -= extra;
    edited += monthly;
    el::io::printLine(
        el::StringFormat{"Combined: {}; edited: {}; difference: {}; negated: {}"_el}.build(
            combined.toString(), edited.toString(), (combined - monthly).toString(), (-combined).toString()));
    const auto minutes = el::CalendarDelta{el::Minutes{60}};
    const auto hour = el::CalendarDelta{el::Hours{1}};
    const auto cancellation = minutes - hour;
    el::io::printLine(
        el::StringFormat{"Same stored parts: {}; cancellation isZero: {}; fixed total: {}"_el}.build(
            minutes == hour, cancellation.isZero(), cancellation.toTimeDeltaOrThrow().toString()));
    el::io::printLine(
        el::StringFormat{"Subtracting identical parts isZero: {}"_el}.build((combined - combined).isZero()));
}

}
