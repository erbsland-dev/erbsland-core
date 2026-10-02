// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Apply mixed units in order and observe month-end and leap-day clamping.
/// @notest{Compiled and executed documentation demo.}
void apply() {
    const auto start =
        el::DateTime{el::Date{el::Year{2024}, el::Month{1}, el::Day{30}}, el::Time{el::Hour{23}, el::Minute{30}}};
    const auto change = el::CalendarDelta{el::CalendarDeltaParts{
        .hours = el::Hours{2}, .days = el::Days{1}, .months = el::Months{1}, .years = el::Years{1}}};
    auto step = start.added(el::CalendarDelta{el::Hours{2}});
    el::io::printLine(el::StringFormat{"After hours: {}"_el}.build(step.toString()));
    step.add(el::CalendarDelta{el::Days{1}});
    el::io::printLine(el::StringFormat{"After days: {}"_el}.build(step.toString()));
    step.addOrThrow(el::CalendarDelta{el::Months{1}});
    el::io::printLine(el::StringFormat{"After months: {}"_el}.build(step.toString()));
    step += el::CalendarDelta{el::Years{1}};
    el::io::printLine(
        el::StringFormat{"After years: {}; combined: {}"_el}.build(
            step.toString(), start.addedOrThrow(change).toString()));
    const auto monthEnd =
        el::DateTime{el::Date{el::Year{2024}, el::Month{1}, el::Day{31}}, el::Time{el::Hour{9}, el::Minute{}}};
    const auto month = el::CalendarDelta{el::Months{1}};
    const auto next = monthEnd + month;
    el::io::printLine(
        el::StringFormat{"Month end: {}; after month: {}"_el}.build(monthEnd.toString(), next.toString()));
    el::io::printLine(el::StringFormat{"Subtract month: {}"_el}.build(next.subtracted(month).toString()));
    el::io::printLine(
        el::StringFormat{"Two at once: {}; two separately: {}"_el}.build(
            (monthEnd + (month + month)).toString(), ((monthEnd + month) + month).toString()));
    el::io::printLine(
        el::StringFormat{"Leap day plus year: {}"_el}.build((next + el::CalendarDelta{el::Years{1}}).toString()));
}

}
