// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DurationDemos.hpp"

#include <erbsland/time/all.hpp>

#include <array>
#include <utility>

namespace demo {

/// Split totals into components with a chosen largest unit and retain signed parts.
/// @notest{Compiled and executed documentation demo.}
void parts() {
    const auto interval = el::Duration{el::Duration::Parts{
        .seconds = el::Seconds{5}, .minutes = el::Minutes{4}, .hours = el::Hours{3}, .days = el::Days{9}}};
    el::io::printLine(
        el::StringFormat{"Total: {} s; components: {} d, {} h, {} m, {} s"_el}.build(
            interval.toSeconds().toRawValue(),
            interval.days().toRawValue(),
            interval.hours().toRawValue(),
            interval.minutes().toRawValue(),
            interval.seconds().toRawValue()));
    const auto choices = std::array{
        std::pair{"Seconds"_el, el::DurationPart::Seconds},
        std::pair{"Minutes"_el, el::DurationPart::Minutes},
        std::pair{"Hours"_el, el::DurationPart::Hours},
        std::pair{"Days"_el, el::DurationPart::Days},
        std::pair{"Weeks"_el, el::DurationPart::Weeks}};
    for (const auto &[name, largest] : choices) {
        const auto parts = interval.parts(largest);
        el::io::printLine(
            el::StringFormat{"{}: {} w, {} d, {} h, {} m, {} s"_el}.build(
                name,
                parts.weeks.toRawValue(),
                parts.days.toRawValue(),
                parts.hours.toRawValue(),
                parts.minutes.toRawValue(),
                parts.seconds.toRawValue()));
    }
    const auto negative = -interval;
    const auto parts = negative.parts();
    el::io::printLine(
        el::StringFormat{"Negative: {} d, {} h, {} m, {} s; rebuilt equal: {}"_el}.build(
            parts.days.toRawValue(),
            parts.hours.toRawValue(),
            parts.minutes.toRawValue(),
            parts.seconds.toRawValue(),
            el::Duration{parts} == negative));
}

}
