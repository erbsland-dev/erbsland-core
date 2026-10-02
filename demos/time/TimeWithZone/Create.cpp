// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Keep a daily wall-clock reading with its intended zone before choosing a date.
/// Component accessors retain fractional seconds; equality compares the stored time and zone.
/// @notest{Compiled and executed documentation demo.}
void create() {
    const auto movement = "Αντάντε"_el;
    const auto reading = el::Time{el::Hour{9}, el::Minute{15}, el::Second{12}, el::Nanoseconds{123456789}};
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto rehearsal = el::TimeWithZone{reading, zone};
    const auto fixed = el::TimeWithZone{reading, el::TimeZone{el::Hours{2}}};
    const auto local = el::TimeWithZone{reading, el::TimeZone::local()};

    // A time alone defaults to UTC; default construction is midnight UTC.
    el::io::printLine(
        el::StringFormat{"{}: {}\nDefault: {}\nTime-only: {}"_el}.build(
            movement, rehearsal, el::TimeWithZone{}, el::TimeWithZone{reading}));
    el::io::printLine(
        el::StringFormat{"Hour: {}\nMinute: {}\nSecond: {}\nMs fraction: {}\nNs fraction: {}"_el}.build(
            rehearsal.hour().toRawValue(),
            rehearsal.minute().toRawValue(),
            rehearsal.second().toRawValue(),
            rehearsal.millisecondFraction().toRawValue(),
            rehearsal.nanosecondFraction().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Reading retained: {}\nNamed zone: {}\nLocal origin: {}"_el}.build(
            rehearsal.time() == reading, rehearsal.timeZone().isNamed(), local.timeZone().isLocalTime()));
    el::io::printLine(
        el::StringFormat{"Equal copy: {}\nEqual to fixed-zone reading: {}"_el}.build(
            rehearsal == el::TimeWithZone{reading, zone}, rehearsal == fixed));
}

}
