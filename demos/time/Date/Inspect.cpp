// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Compare dates and extract fields for a calendar view.
/// @notest{Compiled and executed documentation demo.}
void inspect() {
    const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
    const auto invalid = el::Date{};
    el::io::printLine(
        el::StringFormat{"Valid: {}; invalid sorts first: {}; invalid dates equal: {}"_el}.build(
            date.isValid(), invalid < el::Date::first(), invalid == el::Date{}));
    el::io::printLine(el::StringFormat{"Next observation is later: {}"_el}.build(date < date.next()));
    el::io::printLine(
        el::StringFormat{"At first boundary: {}; at last boundary: {}"_el}.build(
            el::Date::first().isFirst(), el::Date::last().isLast()));
    el::io::printLine(
        el::StringFormat{"Date exists: {}"_el}.build(
            el::Date::exists(el::Year{2028}, el::Month::february(), el::Day{29})));

    // An invalid date's fallback fields cannot replace its validity check.
    const auto fallback = invalid.parts();
    el::io::printLine(
        el::StringFormat{"Invalid fallback: {}-{}-{}; still invalid: {}"_el}.build(
            fallback.year.toValue(), fallback.month.toValue(), fallback.day.toValue(), !invalid.isValid()));

    // Extract related fields together when preparing a calendar view.
    const auto parts = date.parts();
    el::io::printLine(
        el::StringFormat{"Year: {}; month: {}; day: {}"_el}.build(
            parts.year.toValue(), parts.month.toValue(), parts.day.toValue()));
    el::io::printLine(
        el::StringFormat{"Individual fields: {} / {} / {}"_el}.build(
            date.year().toValue(), date.month().toValue(), date.day().toValue()));
    el::io::printLine(
        el::StringFormat{"Day of year: {}; weekday: {} (index {})"_el}.build(
            date.dayOfYear().toValue(), date.dayOfWeek().toString(), date.dayOfWeek().toValue()));
}

}
