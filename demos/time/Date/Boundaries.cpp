// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DateDemos.hpp"

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/time/all.hpp>

namespace demo {

/// Choose saturation or explicit failure at date boundaries.
/// @notest{Compiled and executed documentation demo.}
void boundaries() {
    auto end = el::Date::last();
    el::io::printLine(el::StringFormat{"Adding a day would saturate: {}"_el}.build(end.wouldAddSaturate(el::Days{1})));
    end.add(el::Days{1});
    el::io::printLine(el::StringFormat{"Saturated result: {}"_el}.build(end));
    try {
        end.addOrThrow(el::Months{1});
    } catch (const el::OverflowError &) {
        el::io::printLine(
            el::StringFormat{"Rejected a month beyond the supported range; date remains {}."_el}.build(end));
    }
    el::io::printLine(el::StringFormat{"First minus one year: {}"_el}.build(el::Date::first().added(el::Years{-1})));
    el::io::printLine(
        el::StringFormat{"Next beyond last valid: {}; previous before first valid: {}"_el}.build(
            end.next().isValid(), el::Date::first().previous().isValid()));
    const auto invalid = el::Date{};
    el::io::printLine(
        el::StringFormat{"Invalid remains invalid: {}; invalid saturation query: {}; invalid distance: {}"_el}.build(
            invalid.addedOrThrow(el::Days{1}).isValid(),
            invalid.wouldAddSaturate(el::Days{1}),
            invalid.daysTo(end).toRawValue()));
}

}
