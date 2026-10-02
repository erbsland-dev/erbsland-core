// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Navigate across calendar boundaries with enough context to carry the year and month.
/// @notest{Compiled and executed documentation demo.}
void navigate() {
    const auto year = el::Year{2024};
    const auto month = el::Month::december();
    const auto nextMonth = month.next(year);
    const auto previousMonth = nextMonth.month.previous(nextMonth.year);
    const auto nextDay = el::Day{31}.next(year, month);
    const auto previousDay = nextDay.day.previous(nextDay.year, nextDay.month);
    el::io::printLine(
        el::StringFormat{"Next month: {}-{}; back: {}-{}; next day: {}; back: {}"_el}.build(
            nextMonth.year.toValue(),
            nextMonth.month.toValue(),
            previousMonth.year.toValue(),
            previousMonth.month.toValue(),
            el::Date::fromParts(nextDay.year, nextDay.month, nextDay.day).toString(),
            el::Date::fromParts(previousDay.year, previousDay.month, previousDay.day).toString()));
    el::io::printLine(
        el::StringFormat{"Year next/previous: {}/{}; final year has next: {}; first year has previous: {}"_el}.build(
            year.next().toValue(),
            year.previous().toValue(),
            el::Year::last().hasNext(),
            el::Year::first().hasPrevious()));
    el::io::printLine(
        el::StringFormat{"Month next/previous available: {}/{}; day next/previous available: {}/{}"_el}.build(
            month.hasNext(el::Year::last()),
            el::Month::january().hasPrevious(el::Year::first()),
            el::Day{31}.hasNext(el::Year::last(), month),
            el::Day{1}.hasPrevious(el::Year::first(), el::Month::january())));
}

}
