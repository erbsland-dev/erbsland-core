// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Construct, validate, and compare UTC instants for recorded events.
/// @notest{Compiled and executed documentation demo.}
void create() {
    const auto eventName = "Μάθημα αστρονομίας"_el;
    const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 1);
    const auto start = el::Timestamp{date, el::Time{el::Hour{12}, el::Minute{30}}};
    const auto restored = el::Timestamp::fromDaysAndNanosecondsOrThrow(start.dateAsDays(), start.timeAsNanoseconds());
    el::io::printLine(el::StringFormat{"Event: {}; start: {}; restored: {}"_el}.build(eventName, start, restored));

    // A successfully decoded invalid sentinel is different from malformed input.
    const auto invalid = el::Timestamp::fromDaysAndNanoseconds(el::Days{-1}, el::Nanoseconds{});
    const auto malformed = el::Timestamp::fromDaysAndNanoseconds(el::Days{}, el::Nanoseconds{86'400'000'000'000});
    el::io::printLine(
        el::StringFormat{"Sentinel decoded: {}; sentinel valid: {}; oversized input decoded: {}"_el}.build(
            invalid.has_value(), invalid->isValid(), malformed.has_value()));
    el::io::printLine(
        el::StringFormat{"Default valid: {}; invalid sorts first: {}; invalid values equal: {}"_el}.build(
            el::Timestamp{}.isValid(), el::Timestamp{} < start, el::Timestamp{} == *invalid));
    el::io::printLine(
        el::StringFormat{"Epoch: {}; first: {}; last: {}"_el}.build(
            el::Timestamp::epoch(), el::Timestamp::first(), el::Timestamp::last()));
    // Avoid capturing a changing clock reading in the documentation output.
    el::io::printLine(el::StringFormat{"Current wall clock is valid: {}"_el}.build(el::Timestamp::now().isValid()));
    try {
        const auto unexpected = el::Timestamp::fromDaysAndNanosecondsOrThrow(el::Days{3'652'425}, el::Nanoseconds{});
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
    } catch (const el::err::ParameterError &) {
        el::io::printLine("Checked construction rejects an out-of-range day count."_el);
    }
}

}
