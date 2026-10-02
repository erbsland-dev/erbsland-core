// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Convert signed epoch ticks while preserving the meaning of negative fractions.
/// @notest{Compiled and executed documentation demo.}
void ticks() {
    const auto before = el::Timestamp::fromTicksOrThrow(el::Milliseconds{-250}, el::TimeEpoch::Posix);
    const auto [seconds, fraction] = before.toSecondsAndFractionsOrThrow(el::TimeEpoch::Posix);
    el::io::printLine(
        el::StringFormat{"Before POSIX epoch: {}; scalar seconds: {}; milliseconds: {}"_el}.build(
            before,
            before.toTicksOrThrow<el::Seconds>(el::TimeEpoch::Posix).toRawValue(),
            before.toTicksOrThrow<el::Milliseconds>(el::TimeEpoch::Posix).toRawValue()));
    el::io::printLine(
        el::StringFormat{"Floor seconds: {}; fraction: {}; split round trip: {}"_el}.build(
            seconds.toRawValue(),
            fraction.toRawValue(),
            el::Timestamp::fromTicksOrThrow(seconds, fraction, el::TimeEpoch::Posix) == before));
    el::io::printLine(
        el::StringFormat{"Microseconds: {}; nanoseconds: {}; DateTime accepts signed ticks: {}"_el}.build(
            before.toTicksOrThrow<el::Microseconds>(el::TimeEpoch::Posix).toRawValue(),
            before.toTicksOrThrow<el::Nanoseconds>(el::TimeEpoch::Posix).toRawValue(),
            el::DateTime::fromTicks(el::Milliseconds{-250}, el::TimeEpoch::Posix).has_value()));
    el::io::printLine(
        el::StringFormat{"Last instant fits Core nanoseconds: {}; invalid fraction accepted: {}"_el}.build(
            el::Timestamp::last().toTicks<el::Nanoseconds>().has_value(),
            el::Timestamp::fromTicks(el::Seconds{}, el::Nanoseconds{1'000'000'000}).has_value()));
    try {
        const auto unexpected = el::Timestamp::last().toTicksOrThrow<el::Nanoseconds>();
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toRawValue()));
    } catch (const el::err::OutOfRangeError &) {
        el::io::printLine("Checked tick conversion rejects overflow."_el);
    }
    // Native time_t has platform-dependent range and discards fractional seconds.
    const auto posix = el::Timestamp::epoch(el::TimeEpoch::Posix);
    el::io::printLine(
        el::StringFormat{"POSIX time_t: {}; native round trip: {}; negative fraction floors to: {}"_el}.build(
            posix.toTimeT(), el::Timestamp::fromTimeT(posix.toTimeT()) == posix, before.toTimeT()));
}

}
