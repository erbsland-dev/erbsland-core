// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDeltaDemos.hpp"

#include <erbsland/time/all.hpp>

#include <chrono>

namespace demo {

/// Construct nanosecond intervals from typed amounts, literals, and chrono durations.
/// @notest{Compiled and executed documentation demo.}
void create() {
    using namespace el::time::literals;

    const auto component = "reloj de arena"_el;
    const auto typed = el::TimeDelta{el::Milliseconds{1250}};
    const auto literal = el::TimeDelta{1250_ms};
    const auto chrono = el::TimeDelta{std::chrono::microseconds{1'250'001}};
    el::io::printLine(
        el::StringFormat{"{}: typed {}; literal {}; chrono {}"_el}.build(
            component, typed.toString(), literal.toString(), chrono.toString()));
    el::io::printLine(
        el::StringFormat{"Default is zero: {}; zero factory: {}"_el}.build(
            el::TimeDelta{}.isZero(), el::TimeDelta::zero().isZero()));
}

}
