// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Display an intended daily time with UTC, fixed, named, or system-local zone information.
/// Compact output omits local-origin zones and does not identify a dated instant.
/// @notest{Compiled and executed documentation demo.}
void display() {
    const auto reading = el::Time{el::Hour{9}, el::Minute{15}};
    for (
        const auto zone :
        {el::TimeZone::utc(),
            el::TimeZone{el::Hours{5}, el::Minutes{30}},
            el::TimeZone::fromNameOrThrow("Europe/Athens"_el),
            el::TimeZone::local()}) {
        const auto rehearsal = el::TimeWithZone{reading, zone};
        el::io::printLine(
            el::StringFormat{"Intended time: {}\nLocal origin: {}"_el}.build(
                rehearsal.toString(), rehearsal.timeZone().isLocalTime()));
    }
    // Second-level fixed offsets remain visible when needed.
    el::io::printLine(
        el::StringFormat{"Second-level offset: {}"_el}.build(
            el::TimeWithZone{reading, el::TimeZone{el::Hours{-3}, el::Minutes{-30}, el::Seconds{-15}}}));
}

}
