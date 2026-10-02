// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaDemos.hpp"

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/time/all.hpp>

#include <array>

namespace demo {

/// Choose fixed-unit factories and reject nanosecond conversion overflow.
/// @notest{Compiled and executed documentation demo.}
void factories() {
    const auto intervals = std::array{
        el::TimeDelta::nanoseconds(1),
        el::TimeDelta::microseconds(1),
        el::TimeDelta::milliseconds(1),
        el::TimeDelta::seconds(1),
        el::TimeDelta::minutes(1),
        el::TimeDelta::hours(1),
        el::TimeDelta::days(1),
        el::TimeDelta::weeks(1)};
    // Compare the output for each choice using the same input.
    for (const auto interval : intervals) {
        el::io::printLine(interval.toString());
    }

    // Each coarser unit also has a checked factory with the same conversion policy.
    const auto checkedUnits = std::array{
        el::TimeDelta::microsecondsOrThrow(1),
        el::TimeDelta::millisecondsOrThrow(1),
        el::TimeDelta::secondsOrThrow(1),
        el::TimeDelta::minutesOrThrow(1),
        el::TimeDelta::hoursOrThrow(1),
        el::TimeDelta::daysOrThrow(1),
        el::TimeDelta::weeksOrThrow(1)};
    el::io::printLine(el::StringFormat{"Checked week: {}"_el}.build(checkedUnits.back().toString()));

    // A value that fits in whole seconds can exceed the nanosecond range.
    const auto large = el::Seconds::maximum();
    const auto saturated = el::TimeDelta::seconds(large.toRawValue());
    el::io::printLine(
        el::StringFormat{"Would saturate: {}; stored ns: {}"_el}.build(
            large.wouldConvertSaturate<el::Nanoseconds>(), saturated.toNanoseconds().toRawValue()));
    try {
        const auto checked = el::TimeDelta::secondsOrThrow(large.toRawValue());
        el::io::printLine(checked.toString());
    } catch (const el::err::OverflowError &) {
        el::io::printLine("Checked factory rejected overflow."_el);
    }
}

}
