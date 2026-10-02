// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarDeltaDemos.hpp"

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/time/all.hpp>

namespace demo {

/// Detect intermediate overflow and choose saturating or throwing application.
/// @notest{Compiled and executed documentation demo.}
void boundaries() {
    const auto last = el::DateTime::last();
    const auto tick = el::CalendarDelta{el::Nanoseconds{1}};
    el::io::printLine(
        el::StringFormat{"Upper overflow: {}; lower overflow: {}; clamped high: {}"_el}.build(
            last.wouldAddSaturate(tick), el::DateTime::first().wouldSubtractSaturate(tick), last.added(tick) == last));
    // The first step overflows even though a later day step moves back into range.
    const auto mixed = el::CalendarDelta{el::CalendarDeltaParts{.seconds = el::Seconds{1}, .days = el::Days{-1}}};
    el::io::printLine(
        el::StringFormat{"Intermediate overflow: {}; saturating result: {}"_el}.build(
            last.wouldAddSaturate(mixed), last.added(mixed).toString()));
    try {
        const auto checked = last.addedOrThrow(mixed);
        el::io::printLine(checked.toString());
    } catch (const el::err::OverflowError &) {
        el::io::printLine("Checked application rejects the first overflowing step."_el);
    }
    auto changed = last;
    changed.subtractOrThrow(el::CalendarDelta{el::Days{1}});
    changed.subtract(el::CalendarDelta{el::Months{1}});
    el::io::printLine(
        el::StringFormat{"Changed in place: {}; invalid remains invalid: {}"_el}.build(
            changed.toString(), !el::DateTime{}.addedOrThrow(tick).isValid()));
}

}
