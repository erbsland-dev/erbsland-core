// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeLiteralsDemos.hpp"

#include <erbsland/time/all.hpp>
#include <erbsland/time/Literals.hpp>

namespace demo {

/// Keep units visible in an interval expression by importing the time literals locally.
/// @notest{Compiled and executed documentation demo.}
void scope() {
    using namespace erbsland::time::literals;

    const auto creature = "Drago delle nuvole"_el;
    const auto interval = el::TimeDelta{250_ms};
    el::io::printLine(el::StringFormat{"{}: observation interval {}"_el}.build(creature, interval.toString()));
}

}
