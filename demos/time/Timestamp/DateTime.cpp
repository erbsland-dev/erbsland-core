// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Retain an instant while choosing or discarding its display zone.
/// @notest{Compiled and executed documentation demo.}
void dateTime() {
    const auto local = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+02:00"_el);
    const auto event = el::Timestamp::fromDateTimeOrThrow(local);
    const auto utc = event.toDateTimeOrThrow();
    el::io::printLine(el::StringFormat{"Local input: {}; UTC conversion: {}"_el}.build(local, utc));
    el::io::printLine(
        el::StringFormat{"Instant unchanged: {}; DateTime equality: {}; convertible: {}"_el}.build(
            utc.toUtc() == local.toUtc(), utc == local, event.isValidDateTime()));
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
    el::io::printLine(el::StringFormat{"Chosen display: {}"_el}.build(utc.toTimeZone(zone)));
    el::io::printLine(
        el::StringFormat{"Invalid input converts: {}; invalid timestamp converts: {}"_el}.build(
            el::Timestamp::fromDateTime(el::DateTime{}).has_value(), el::Timestamp{}.toDateTime().has_value()));
    try {
        const auto unexpected = el::Timestamp{}.toDateTimeOrThrow();
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
    } catch (const el::err::OutOfRangeError &) {
        el::io::printLine("An invalid timestamp has no dated instant."_el);
    }
}

}
