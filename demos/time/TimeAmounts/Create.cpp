// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeAmountsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Construct signed quantities and keep their units separate from calendar positions and raw counts.
/// @notest{Compiled and executed documentation demo.}
void create() {
    using namespace erbsland::time::literals;

    const auto creature = "Fenice del bosco"_el;
    const auto empty = el::Milliseconds{};
    const auto delay = el::Milliseconds{250};
    const auto correction = el::Milliseconds{-25};
    const auto fromLiteral = 250_ms;
    el::io::printLine(
        el::StringFormat{"{}: delay {} ms; default zero: {}; positive: {}; negative: {}; literal equal: {}"_el}.build(
            creature,
            delay.toRawValue(),
            empty.isZero(),
            delay.isPositive(),
            correction.isNegative(),
            delay == fromLiteral));

    // toValue() retains saturating integer behavior, but no longer carries the time unit.
    const auto raw = delay.toRawValue();
    const auto value = delay.toValue();
    el::io::printLine(el::StringFormat{"Native count: {}; saturating count: {}"_el}.build(raw, value.toRawValue()));

    const auto position = el::Day{15};
    const auto quantity = el::Days{15};
    const auto clockHour = el::Hour{9};
    const auto elapsedHours = el::Hours{9};
    const auto calendarMonth = el::Month::march();
    const auto monthCount = el::Months{3};
    el::io::printLine(
        el::StringFormat{
            "Day field: {}; day count: {}; hour field: {}; hour count: {}; month field: {}; month count: {}"_el}
            .build(
                position.toRawValue(),
                quantity.toRawValue(),
                clockHour.toRawValue(),
                elapsedHours.toRawValue(),
                calendarMonth.toRawValue(),
                monthCount.toRawValue()));

    const auto whole = el::Duration{el::Minutes{2}};
    const auto precise = el::TimeDelta{delay};
    const auto calendar = el::CalendarDelta{monthCount};
    el::io::printLine(
        el::StringFormat{"Whole interval: {} s; precise interval: {}; calendar change: {}"_el}.build(
            whole.toSeconds().toRawValue(), precise.toString(), calendar.toString()));
}

}
