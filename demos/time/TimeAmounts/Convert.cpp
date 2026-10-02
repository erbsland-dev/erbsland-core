// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeAmountsDemos.hpp"

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/time/all.hpp>

#include <initializer_list>

namespace demo {

/// Choose a common fixed unit, recognizing truncation and checking finer-unit overflow.
/// @notest{Compiled and executed documentation demo.}
void convert() {
    const auto delay = el::Seconds{2}.converted<el::Milliseconds>();
    const auto week = el::Weeks{1}.converted<el::Days>();
    el::io::printLine(
        el::StringFormat{"2 s: {} ms; one week: {} days; one day: {} seconds"_el}.build(
            delay.toRawValue(), week.toRawValue(), el::Days{1}.converted<el::Seconds>().toRawValue()));

    for (const auto input : {1500, -1500, 999, -999}) {
        const auto amount = el::Milliseconds{input};
        const auto seconds = amount.convertedOrThrow<el::Seconds>();
        el::io::printLine(
            el::StringFormat{"{} ms: {} whole seconds; would saturate: {}"_el}.build(
                input, seconds.toRawValue(), amount.wouldConvertSaturate<el::Seconds>()));
    }

    const auto large = el::Seconds::maximum();
    const auto saturated = large.converted<el::Nanoseconds>();
    el::io::printLine(
        el::StringFormat{"Maximum seconds to ns would saturate: {}; clamped result: {}"_el}.build(
            large.wouldConvertSaturate<el::Nanoseconds>(), saturated.toRawValue()));
    try {
        const auto checked = large.convertedOrThrow<el::Nanoseconds>();
        el::io::printLine(el::StringFormat{"Checked result: {}"_el}.build(checked.toRawValue()));
    } catch (const el::err::OverflowError &) {
        el::io::printLine("Checked conversion rejected overflow."_el);
    }
    // Months and years cannot be converted to fixed units or to each other.
}

}
