// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/time/all.hpp>

namespace demo {

/// Convert positions to offsets from their minimum and handle invalid offsets.
/// @notest{Compiled and executed documentation demo.}
void amounts() {
    const auto firstDay = el::Day{1};
    const auto firstMonth = el::Month{1};
    const auto hour = el::Hour{9};
    el::io::printLine(
        el::StringFormat{"Day 1 -> {} days; month 1 -> {} months; hour 9 -> {} hours"_el}.build(
            firstDay.toAmount().toRawValue(), firstMonth.toAmount().toRawValue(), hour.toAmount().toRawValue()));
    const auto day = el::Day::fromAmount(el::Days{14});
    const auto month = el::Month::fromAmountOrThrow(el::Months{5});
    el::io::printLine(
        el::StringFormat{"Offset 14 -> day {}; offset 5 -> month {}; day equals offset 14: {}"_el}.build(
            day.toValue(), month.toValue(), day == el::Days{14}));
    el::io::printLine(
        el::StringFormat{"Day plus 2: {}; negative offset clamped: {}"_el}.build(
            (day + el::Days{2}).toValue(), el::Day::fromAmount(el::Days{-1}).toValue()));
    try {
        const auto invalid = el::Month::fromAmountOrThrow(el::Months{12});
        el::io::printLine(el::StringFormat{"Month: {}"_el}.build(invalid.toValue()));
    } catch (const el::err::OutOfRangeError &) {
        el::io::printLine("Month offsets must be in 0..11."_el);
    }
}

}
