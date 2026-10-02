// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Choose UTC, a fixed offset, or a supported named zone with explicit lookup failure handling.
/// Fixed-offset components have independent signs; total durations discard complete 24-hour rotations.
/// @notest{Compiled and executed documentation demo.}
void create() {
    const auto movement = "Αντάντε"_el;
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    el::io::printLine(
        el::StringFormat{"{}: named zone {}\nDefault UTC: {}\nUtc() UTC: {}"_el}.build(
            movement, zone.name(), el::TimeZone{}.isUtc(), el::TimeZone::utc().isUtc()));
    const auto input = "Unknown/Composition"_el;
    if (const auto parsed = el::TimeZone::fromName(input); parsed.has_value()) {
        el::io::printLine(el::StringFormat{"Chosen zone: {}"_el}.build(parsed->name()));
    } else {
        el::io::printLine("Unknown zone: choose a supported name before resolving local input."_el);
    }
    try {
        [[maybe_unused]] const auto rejected = el::TimeZone::fromNameOrThrow(input);
    } catch (const el::err::ParseError &) {
        el::io::printLine("Throwing lookup reports ParseError."_el);
    }
    el::io::printLine(
        el::StringFormat{"Athens supported: {}\nUnknown supported: {}"_el}.build(
            el::TimeZone::isValidName("Europe/Athens"_el), el::TimeZone::isValidName(input)));

    // Apply the sign to each nonzero component of a negative offset.
    const auto negative = el::TimeZone{el::Hours{-3}, el::Minutes{-30}, el::Seconds{-15}};
    const auto mixed = el::TimeZone{el::Hours{-3}, el::Minutes{30}};
    const auto clamped = el::TimeZone{el::Hours{30}, el::Minutes{90}, el::Seconds{90}};
    const auto normalized = el::TimeZone{el::Duration{el::Hours{27}}};
    el::io::printLine(
        el::StringFormat{"Negative seconds: {}\nMixed signs: {}\nClamped parts: {}\nNormalized total: {}"_el}.build(
            negative.staticOffset().toSeconds().toRawValue(),
            mixed.staticOffset().toSeconds().toRawValue(),
            clamped.staticOffset().toSeconds().toRawValue(),
            normalized.staticOffset().toSeconds().toRawValue()));
    for (const auto name : {"Z"_el, "GMT"_el, "+0530"_el, "UTC+05:00"_el, "Etc/GMT+5"_el}) {
        const auto parsed = el::TimeZone::fromNameOrThrow(name);
        const auto instant =
            el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 7, 15), el::Time{}}.toTimeZone(parsed);
        el::io::printLine(
            el::StringFormat{"{}: offset seconds {}"_el}.build(name, instant.timeOffset().toSeconds().toRawValue()));
    }
}

}
