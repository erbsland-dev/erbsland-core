// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaOutputDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Normalize fixed display components without merging calendar months or changing stored parts.
/// @notest{Compiled and executed documentation demo.}
void calendar() {
    const auto change = el::CalendarDelta{el::CalendarDeltaParts{
        .minutes = el::Minutes{90}, .hours = el::Hours{-1}, .months = el::Months{-14}, .years = el::Years{1}}};
    el::io::printLine(
        el::StringFormat{"Short: {}; long: {}"_el}.build(
            change.toString(), change.toString(el::TimeDeltaFormat::longUnits())));
    el::io::printLine(
        el::StringFormat{"Stored minutes: {}; stored hours: {}"_el}.build(
            change.minutes().toRawValue(), change.hours().toRawValue()));
    const auto cancelling =
        el::CalendarDelta{el::CalendarDeltaParts{.minutes = el::Minutes{60}, .hours = el::Hours{-1}}};
    el::io::printLine(
        el::StringFormat{"Cancellation: {}; all stored parts zero: {}"_el}.build(
            cancelling.toString(), cancelling.isZero()));
    const auto wide = el::CalendarDelta{el::Weeks::maximum()};
    el::io::printLine(
        el::StringFormat{"Wide fixed total: {}; fits TimeDelta: {}"_el}.build(
            wide.toString(), wide.toTimeDelta().has_value()));
    const auto weeksOnly = el::TimeDeltaFormat{}.setSmallestUnit(el::TimeDeltaUnit::Weeks);
    el::io::printLine(
        el::StringFormat{"Weeks-only fixed display retains calendar units: {}"_el}.build(change.toString(weeksOnly)));
}

}
