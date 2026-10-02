// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DurationDemos.hpp"

#include <erbsland/time/all.hpp>

#include <chrono>
#include <initializer_list>

namespace demo {

/// Build whole-second intervals from typed amounts, parts, and chrono values.
/// @notest{Compiled and executed documentation demo.}
void create() {
    const auto prototype = "試作センサー"_el;
    const auto empty = el::Duration{};
    const auto timeout = el::Duration{el::Seconds{30}};
    const auto warmup = el::Duration{el::Minutes{2}};
    const auto fixed = el::Duration{el::Days{2}};
    el::io::printLine(
        el::StringFormat{"{}: timeout {} s; warmup {} s; fixed interval {} s"_el}.build(
            prototype,
            timeout.toSeconds().toRawValue(),
            warmup.toSeconds().toRawValue(),
            fixed.toSeconds().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Default is zero: {}; zero factory matches: {}"_el}.build(
            empty.isZero(), empty == el::Duration::zero()));

    // Components are combined as fixed amounts, even beyond customary field limits.
    const auto combined = el::Duration{el::Duration::Parts{.seconds = el::Seconds{5}, .minutes = el::Minutes{90}}};
    el::io::printLine(el::StringFormat{"90 minutes and 5 seconds: {} s"_el}.build(combined.toSeconds().toRawValue()));

    // Sub-second inputs truncate toward zero, including negative inputs.
    for (const auto milliseconds : {2500, -2500, 999, -999}) {
        const auto typed = el::Duration{el::Milliseconds{milliseconds}};
        const auto chrono = el::Duration{std::chrono::milliseconds{milliseconds}};
        el::io::printLine(
            el::StringFormat{"{} ms: typed {} s; chrono {} s"_el}.build(
                milliseconds, typed.toSeconds().toRawValue(), chrono.toSeconds().toRawValue()));
    }
}

}
