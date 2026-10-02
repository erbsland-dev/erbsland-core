// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeAmountsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Apply month and year quantities in calendar context, and preserve combined changes in CalendarDelta.
/// @notest{Compiled and executed documentation demo.}
void calendar() {
    const auto start = el::Date{el::Year{2026}, el::Month::january(), el::Day{31}};
    const auto nextMonth = start.added(el::Months{1});
    const auto fixedDays = start.added(el::Days{30});
    const auto leapDay = el::Date{el::Year{2024}, el::Month::february(), el::Day{29}};
    el::io::printLine(
        el::StringFormat{"Start: {}; plus one month: {}; plus 30 days: {}; leap day plus one year: {}"_el}.build(
            start.toString(), nextMonth.toString(), fixedDays.toString(), leapDay.added(el::Years{1}).toString()));

    const auto change = el::CalendarDelta{el::Months{1}}.setYears(el::Years{1}).setDays(el::Days{2});
    const auto instant = el::DateTime{start, el::Time{el::Hour{9}, el::Minute{0}}};
    el::io::printLine(
        el::StringFormat{"Combined change: {}; applied: {}"_el}.build(
            change.toString(), instant.added(change).toString()));
}

}
