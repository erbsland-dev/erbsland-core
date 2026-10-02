// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeLiteralsDemos.hpp"

#include <erbsland/time/all.hpp>

#include <chrono>

namespace demo {

/// Literal saturation and later unit conversion are separate steps with separate precision limits.
/// @notest{Compiled and executed documentation demo.}
void limits() {
    using namespace erbsland::time::literals;

    // The token fits unsigned long long, but exceeds the signed amount range.
    const auto oversized = 18'446'744'073'709'551'615_s;
    const auto negated = -18'446'744'073'709'551'615_s;
    el::io::printLine(
        el::StringFormat{"Oversized literal: {}; saturated: {}; then negated: {}"_el}.build(
            oversized.toRawValue(), oversized.isMaximum(), negated.toRawValue()));

    const auto fraction = 1'500_ms;
    const auto seconds = fraction.converted<el::Seconds>();
    const auto negativeSeconds = (-fraction).converted<el::Seconds>();
    el::io::printLine(
        el::StringFormat{"1500 ms in whole seconds: {}; -1500 ms: {}; Duration: {} s"_el}.build(
            seconds.toRawValue(), negativeSeconds.toRawValue(), el::Duration{fraction}.toSeconds().toRawValue()));
    // There is no floating-point suffix: express 1.5 seconds as 1500_ms.

    const auto large = 10'000'000_h;
    el::io::printLine(
        el::StringFormat{"Large hours fit; conversion to nanoseconds would saturate: {}"_el}.build(
            large.wouldConvertSaturate<el::Nanoseconds>()));
    el::io::printLine(
        el::StringFormat{"TimeDelta construction clamps to maximum nanoseconds: {}"_el}.build(
            el::TimeDelta{large}.toNanoseconds().isMaximum()));
    const auto core = 250_ms;
    const auto standard = std::chrono::milliseconds{250};
    el::io::printLine(
        el::StringFormat{"Core and chrono intervals agree: {}"_el}.build(
            el::TimeDelta{core} == el::TimeDelta{standard}));
}

}
