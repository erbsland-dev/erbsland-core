// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Build observation times and wrap elapsed values into a day.
/// @notest{Compiled and executed documentation demo.}
void create() {
    const auto observation = "Observation de Saturne"_el;
    const auto midnight = el::Time{};
    const auto start = el::Time{el::Hour{23}, el::Minute{45}};
    const auto precise = el::Time{el::Hour{23}, el::Minute{45}, el::Second{12}, el::Nanoseconds{123456789}};
    el::io::printLine(el::StringFormat{"{}: {}; precise reading: {}"_el}.build(observation, start, precise));
    el::io::printLine(
        el::StringFormat{"Default: {}; first: {}; last: {}"_el}.build(midnight, el::Time::first(), el::Time::last()));

    // Factories wrap elapsed amounts into a time within the day.
    const auto afterDay = el::Time::fromDurationSinceMidnight(el::Duration{el::Hours{25}});
    const auto beforeMidnight = el::Time::fromDurationSinceMidnight(el::TimeDelta{el::Nanoseconds{-1}});
    el::io::printLine(el::StringFormat{"25 hours: {}; minus one nanosecond: {}"_el}.build(afterDay, beforeMidnight));
    const auto clamped = el::Time{el::Hour{24}, el::Minute{60}, el::Second{60}, el::Nanoseconds{1000000000}};
    el::io::printLine(el::StringFormat{"Clamped fields: {}"_el}.build(clamped));

    // Check external fields before constructing clamped clock parts.
    const auto suppliedHour = 24;
    const auto suppliedMinute = 45;
    const auto suppliedSecond = 0;
    const auto suppliedFraction = el::Nanoseconds{0};
    const auto fieldsAccepted = el::Hour::contains(suppliedHour) && el::Minute::contains(suppliedMinute) &&
        el::Second::contains(suppliedSecond) && suppliedFraction >= el::Nanoseconds{0} &&
        suppliedFraction < el::Nanoseconds{1000000000};
    el::io::printLine(el::StringFormat{"External fields accepted: {}"_el}.build(fieldsAccepted));
}

}
