// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DurationDemos.hpp"

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/time/all.hpp>

#include <initializer_list>

namespace demo {

/// Check whether a whole-second interval fits a nanosecond representation.
/// @notest{Compiled and executed documentation demo.}
void precise() {
    const auto timeout = el::Duration{el::Seconds{30}};
    const auto precise = timeout.toTimeDeltaOrThrow();
    el::io::printLine(
        el::StringFormat{"30 seconds: {} ns; round trip equal: {}"_el}.build(
            precise.toNanoseconds().toRawValue(), precise.toDuration() == timeout));

    // The last exact whole-second values fit; one more second overflows in either direction.
    for (const auto seconds : {9'223'372'036LL, 9'223'372'037LL, -9'223'372'036LL, -9'223'372'037LL}) {
        const auto interval = el::Duration{el::Seconds{seconds}};
        el::io::printLine(
            el::StringFormat{"{} s would saturate: {}"_el}.build(seconds, interval.wouldConvertToTimeDeltaSaturate()));
    }

    // Ten billion seconds fit Duration but exceed TimeDelta's nanosecond range.
    const auto longInterval = el::Duration{el::Seconds{10'000'000'000}};
    el::io::printLine(
        el::StringFormat{"Conversion would saturate: {}; saturated nanoseconds: {}"_el}.build(
            longInterval.wouldConvertToTimeDeltaSaturate(), longInterval.toTimeDelta().toNanoseconds().toRawValue()));
    try {
        const auto checked = longInterval.toTimeDeltaOrThrow();
        el::io::printLine(el::StringFormat{"Checked nanoseconds: {}"_el}.build(checked.toNanoseconds().toRawValue()));
    } catch (const el::err::OverflowError &) {
        el::io::printLine("Checked conversion rejected the overflowing interval."_el);
    }
}

}
