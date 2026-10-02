// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Validate raw fields before clamping and keep positions distinct from quantities.
/// @notest{Compiled and executed documentation demo.}
void construct() {
    const auto observation = "Tangskov"_el;
    const auto rawMonth = 13;
    el::io::printLine(
        el::StringFormat{"{}: month accepted {}; clamped month {}"_el}.build(
            observation, el::Month::contains(rawMonth), el::Month{rawMonth}.toValue()));
    const auto day = el::Day{31};
    const auto date = el::Date::fromParts(el::Year{2024}, el::Month::february(), day);
    el::io::printLine(
        el::StringFormat{"Day in part range: {}; February date valid: {}"_el}.build(
            el::Day::contains(31), date.isValid()));
    auto hour = el::Hour{23};
    hour += el::Hours{2};
    el::io::printLine(
        el::StringFormat{"January first: {}; December last: {}; June < July: {}; clamped hour: {}"_el}.build(
            el::Month::january().isFirst(),
            el::Month::last().isLast(),
            el::Month::june() < el::Month::july(),
            hour.toValue()));
    el::io::printLine(
        el::StringFormat{"Year first/last: {}/{}; maximum day: {}; default second: {}"_el}.build(
            el::Year::first().toValue(),
            el::Year::last().toValue(),
            el::Day::lastMaximum().toValue(),
            el::Second{}.toValue()));
}

}
