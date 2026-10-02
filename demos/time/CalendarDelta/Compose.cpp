// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Build a calendar change from independent amounts and inspect its stored parts.
/// @notest{Compiled and executed documentation demo.}
void compose() {
    const auto survey = "Tangskov"_el;
    const auto zero = el::CalendarDelta{};
    const auto monthly = el::CalendarDelta{el::Months{1}};
    const auto visit =
        el::CalendarDelta{el::CalendarDeltaParts{.hours = el::Hours{2}, .days = el::Days{1}, .months = el::Months{1}}};
    auto adjustment = el::CalendarDelta{};
    adjustment.setNanoseconds(el::Nanoseconds{1})
        .setMicroseconds(el::Microseconds{2})
        .setMilliseconds(el::Milliseconds{3})
        .setSeconds(el::Seconds{4})
        .setMinutes(el::Minutes{5})
        .setHours(el::Hours{6})
        .setDays(el::Days{7})
        .setWeeks(el::Weeks{8})
        .setMonths(el::Months{9})
        .setYears(el::Years{10});
    el::io::printLine(
        el::StringFormat{"{}: zero {}; monthly {}; visit {}"_el}.build(
            survey, zero.isZero(), monthly.toString(), visit.toString()));
    // Accessors retain each chosen unit, even when display combines fixed components.
    const auto parts = adjustment.parts();
    el::io::printLine(
        el::StringFormat{"ns {}; us {}; ms {}; s {}; min {}; h {}; d {}; wk {}; mo {}; yr {}"_el}.build(
            adjustment.nanoseconds().toRawValue(),
            adjustment.microseconds().toRawValue(),
            adjustment.milliseconds().toRawValue(),
            adjustment.seconds().toRawValue(),
            adjustment.minutes().toRawValue(),
            adjustment.hours().toRawValue(),
            adjustment.days().toRawValue(),
            adjustment.weeks().toRawValue(),
            adjustment.months().toRawValue(),
            adjustment.years().toRawValue()));
    el::io::printLine(el::StringFormat{"Rebuilt parts equal: {}"_el}.build(el::CalendarDelta{parts} == adjustment));
}

}
