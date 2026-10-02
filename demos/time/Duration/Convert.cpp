// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DurationDemos.hpp"

#include <erbsland/time/all.hpp>

#include <initializer_list>

namespace demo {

/// Pass whole seconds or a signed day and nanosecond split to another API.
/// @notest{Compiled and executed documentation demo.}
void convert() {
    const auto interval =
        el::Duration{el::Duration::Parts{.seconds = el::Seconds{5}, .hours = el::Hours{3}, .days = el::Days{2}}};
    const auto chrono = interval.toStdSeconds();
    el::io::printLine(
        el::StringFormat{"Chrono seconds: {}; round trip equal: {}"_el}.build(
            chrono.count(), el::Duration{chrono} == interval));
    for (const auto value : {interval, -interval}) {
        const auto split = value.toDaysAndNanoseconds();
        el::io::printLine(
            el::StringFormat{"{} s: {} d, {} ns; approximate days: {}"_el}.build(
                value.toSeconds().toRawValue(),
                split.days.toRawValue(),
                split.nanoseconds.toRawValue(),
                value.toDaysWithFractions()));
    }
    // Even the largest duration can be split without converting its entire total to nanoseconds.
    const auto maximum = el::Duration{el::Seconds::maximum()}.toDaysAndNanoseconds();
    el::io::printLine(
        el::StringFormat{"Maximum split: {} d, {} ns"_el}.build(
            maximum.days.toRawValue(), maximum.nanoseconds.toRawValue()));
}

}
