// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarDeltaDemos.hpp"

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/time/all.hpp>

#include <initializer_list>

namespace demo {

/// Convert fixed components exactly and distinguish calendar dependence from overflow.
/// @notest{Compiled and executed documentation demo.}
void convert() {
    const auto fixed = el::CalendarDelta{
        el::CalendarDeltaParts{.milliseconds = el::Milliseconds{250}, .hours = el::Hours{1}, .days = el::Days{2}}};
    const auto month = el::CalendarDelta{el::Months{1}};
    const auto huge = el::CalendarDelta{el::Days{200'000}};
    const auto intermediate = el::CalendarDelta{el::CalendarDeltaParts{
        .nanoseconds = el::Nanoseconds::maximum(), .seconds = el::Seconds{1}, .minutes = el::Minutes{-1}}};
    if (const auto interval = fixed.toTimeDelta()) {
        el::io::printLine(
            el::StringFormat{"Fixed change: {}; precise interval: {}"_el}.build(
                fixed.toString(), interval->toString()));
    }
    el::io::printLine(
        el::StringFormat{"Month valid: {}; optional: {}; large valid: {}; optional: {}"_el}.build(
            month.isValidTimeDelta(),
            month.toTimeDelta().has_value(),
            huge.isValidTimeDelta(),
            huge.toTimeDelta().has_value()));
    el::io::printLine(
        el::StringFormat{"Intermediate sum overflow converts: {}"_el}.build(intermediate.isValidTimeDelta()));
    for (const auto &change : {month, huge, intermediate}) {
        try {
            el::io::printLine(change.toTimeDeltaOrThrow().toString());
        } catch (const el::err::OverflowError &) {
            el::io::printLine("No exact fixed interval fits this change."_el);
        }
    }
}

}
