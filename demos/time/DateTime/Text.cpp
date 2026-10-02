// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Parse ISO calendar text and choose fractional precision and offset output explicitly.
/// @notest{Compiled and executed documentation demo.}
void text() {
    const auto reading = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+03:00"_el);
    const auto format = el::IsoTimeFormatFlags{
        el::IsoTimeFormat::Extended,
        el::IsoTimeFormat::TimePrefix,
        el::IsoTimeFormat::TimeShift,
        el::IsoTimeFormat::UseDotFraction};
    el::io::printLine(
        el::StringFormat{"Display: {}; default ISO: {}; precise ISO: {}"_el}.build(
            reading.toString(), reading.toIsoString(), reading.toIsoString(format, el::DateTimePrecision::Nanosecond)));
    el::io::printLine(
        el::StringFormat{"Default ISO preserves the instant when parsed as UTC: {}"_el}.build(
            el::DateTime::fromIsoStringOrThrow(reading.toIsoString()).toUtc() == reading.toUtc()));
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    const auto local = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00"_el, zone);
    el::io::printLine(
        el::StringFormat{"Local text with a zone: {}; bare text interpreted as UTC: {}"_el}.build(
            local, el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00"_el).isUtc()));
    el::io::printLine(
        el::StringFormat{
            "Offset text with explicit zone accepted: {}; insufficient precision valid: {}; invalid output empty: {}"_el}
            .build(
                el::DateTime::fromIsoString("2026-07-01T14:30:00+03:00"_el, zone).isValid(),
                el::DateTime::fromIsoString("2026-07-01T14:30:00"_el, el::DateTimePrecision::Millisecond).isValid(),
                el::DateTime{}.toIsoString().isEmpty()));
    try {
        const auto unexpected = el::DateTime::fromIsoStringOrThrow("2026-02-30T14:30:00Z"_el);
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
    } catch (const el::err::ParseError &) {
        el::io::printLine("Checked parsing rejects an impossible date."_el);
    }
}

}
