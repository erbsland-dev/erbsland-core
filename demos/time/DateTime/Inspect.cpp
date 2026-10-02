// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Read local fields together and distinguish them from the stored UTC fields.
/// @notest{Compiled and executed documentation demo.}
void inspect() {
    const auto reading = el::DateTime::fromIsoStringOrThrow("2026-07-01T00:30:12.123456789+03:00"_el)
                             .toTimeZone(el::TimeZone::fromNameOrThrow("Europe/Athens"_el));
    const auto parts = reading.parts();
    el::io::printLine(
        el::StringFormat{"Local: {} {}; UTC: {} {}"_el}.build(
            reading.date(), reading.time(), reading.utcDate(), reading.utcTime()));
    el::io::printLine(
        el::StringFormat{"Parts: {}-{}-{} {}:{}:{}; fraction: {} ns"_el}.build(
            parts.year.toValue(),
            parts.month.toValue(),
            parts.day.toValue(),
            parts.hour.toValue(),
            parts.minute.toValue(),
            parts.second.toValue(),
            parts.nanosecondFraction.toRawValue()));
    el::io::printLine(
        el::StringFormat{"Individual fields: {}-{}-{} {}:{}:{}; day of year: {}; weekday: {}"_el}.build(
            reading.year().toValue(),
            reading.month().toValue(),
            reading.day().toValue(),
            reading.hour().toValue(),
            reading.minute().toValue(),
            reading.second().toValue(),
            reading.dayOfYear().toValue(),
            reading.dayOfWeek().toValue()));
    el::io::printLine(
        el::StringFormat{
            "Millisecond fraction: {}; nanosecond fraction: {}; offset (seconds): {}; zone: {}; abbreviation: {}"_el}
            .build(
                reading.millisecondFraction().toRawValue(),
                reading.nanosecondFraction().toRawValue(),
                reading.timeOffset().toSeconds().toRawValue(),
                reading.timeZone().name(),
                reading.timeZoneAbbreviation()));
}

}
