// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CalendarPartsDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Extract named fields and reconstruct complete values with calendar validation and a zone.
/// @notest{Compiled and executed documentation demo.}
void aggregates() {
    const auto date = el::Date{el::Year{2024}, el::Month{6}, el::Day{12}};
    const auto time = el::Time{el::Hour{9}, el::Minute{15}, el::Second{30}, el::Nanoseconds{123'456'789}};
    const auto dateParts = date.parts();
    const auto timeParts = time.parts();
    const auto rebuiltDate = el::Date::fromPartsOrThrow(dateParts.year, dateParts.month, dateParts.day);
    const auto rebuiltTime = el::Time{timeParts.hour, timeParts.minute, timeParts.second, timeParts.nanosecondFraction};
    const auto zone = el::TimeZone::fromNameOrThrow("Europe/Copenhagen"_el);
    const auto instant = el::DateTime{rebuiltDate, rebuiltTime, zone};
    const auto local = instant.parts();
    const auto rebuilt = el::DateTime{
        el::Date::fromParts(local.year, local.month, local.day),
        el::Time{local.hour, local.minute, local.second, local.nanosecondFraction},
        zone};
    el::io::printLine(
        el::StringFormat{"Date/time retained: {}/{}; local/UTC hour: {}/{}; reconstructed: {}"_el}.build(
            rebuiltDate == date,
            rebuiltTime == time,
            local.hour.toValue(),
            instant.utcTime().hour().toValue(),
            rebuilt == instant));
    // Aggregates can hold combinations that require checking when reconstructed.
    const auto impossible = el::DateParts{el::Year{2023}, el::Month::february(), el::Day{29}};
    el::io::printLine(
        el::StringFormat{"Aggregate date valid: {}; fraction retained: {} ns"_el}.build(
            el::Date::fromParts(impossible.year, impossible.month, impossible.day).isValid(),
            local.nanosecondFraction.toRawValue()));
}

}
