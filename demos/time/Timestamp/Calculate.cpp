// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Shift fixed intervals and explicitly choose saturation or an overflow error.
/// @notest{Compiled and executed documentation demo.}
void calculate() {
    const auto start = el::Timestamp::fromIsoStringOrThrow("2026-07-01T23:59:59.900000000Z"_el);
    const auto finish = start.addedOrThrow(el::Milliseconds{250});
    auto adjusted = finish;
    adjusted.subtractOrThrow(el::TimeDelta{el::Milliseconds{250}});
    adjusted.add(el::Duration{el::Seconds{2}});
    adjusted.subtract(el::Seconds{2});
    el::io::printLine(
        el::StringFormat{"Start: {}; finish: {}; restored in place: {}"_el}.build(start, finish, adjusted == start));
    el::io::printLine(
        el::StringFormat{"Operators agree: {}"_el}.build(
            (start + el::Milliseconds{250}) - el::Milliseconds{250} == start));
    auto boundary = el::Timestamp::last();
    el::io::printLine(
        el::StringFormat{"Adding 1 ns saturates: {}; subtracting at first saturates: {}; clamped: {}"_el}.build(
            boundary.wouldAddSaturate(el::Nanoseconds{1}),
            el::Timestamp::first().wouldSubtractSaturate(el::Seconds{1}),
            boundary.added(el::Nanoseconds{1}) == boundary));
    try {
        boundary.addOrThrow(el::Nanoseconds{1});
    } catch (const el::err::OverflowError &) {
        el::io::printLine(
            el::StringFormat{"Checked addition rejected; original unchanged: {}"_el}.build(
                boundary == el::Timestamp::last()));
    }
    el::io::printLine(
        el::StringFormat{"Invalid remains invalid: {}; invalid overflow predicate: {}"_el}.build(
            !el::Timestamp{}.addedOrThrow(el::Seconds{1}).isValid(), el::Timestamp{}.wouldAddSaturate(el::Seconds{1})));
}

}
