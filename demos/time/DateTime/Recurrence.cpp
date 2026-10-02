// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Resolve each occurrence from its intended local date, time, and zone.
/// @notest{Compiled and executed documentation demo.}
void recurrence() {
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto lessonTime = el::TimeWithZone{el::Time{el::Hour{12}, el::Minute{0}}, zone};
    const auto date = el::Date::fromYearMonthDayOrThrow(2026, 3, 28);
    const auto today = el::DateTime{date, lessonTime};
    const auto tomorrow = el::DateTime{date.addedOrThrow(el::Days{1}), lessonTime};
    el::io::printLine(
        el::StringFormat{"Today: {}; next local occurrence: {}; elapsed (seconds): {}"_el}.build(
            today, tomorrow, today.durationTo(tomorrow).toSeconds().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Adding 24 hours instead: {}"_el}.build(today.addedOrThrow(el::Duration{el::Hours{24}})));
}

}
