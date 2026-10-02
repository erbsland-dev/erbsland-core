// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Choose an epoch and a representation that can retain the full calendar range and precision.
/// @notest{Compiled and executed documentation demo.}
void ticks() {
    const auto reading = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+03:00"_el);
    const auto [seconds, fraction] = reading.toSecondsAndFractionsOrThrow(el::TimeEpoch::Posix);
    const auto restored = el::DateTime::fromTicksOrThrow(seconds, fraction, el::TimeEpoch::Posix);
    el::io::printLine(
        el::StringFormat{"POSIX seconds: {}; fraction: {}; full precision round trip: {}"_el}.build(
            seconds.toRawValue(), fraction.toRawValue(), restored == reading.toUtc()));
    el::io::printLine(
        el::StringFormat{"Seconds: {}; milliseconds: {}; microseconds: {}; nanoseconds: {}"_el}.build(
            reading.toSecondsOrThrow(el::TimeEpoch::Posix).toRawValue(),
            reading.toTicksOrThrow<el::Milliseconds>(el::TimeEpoch::Posix).toRawValue(),
            reading.toTicksOrThrow<el::Microseconds>(el::TimeEpoch::Posix).toRawValue(),
            reading.toNanosecondsOrThrow(el::TimeEpoch::Posix).toRawValue()));
    const auto nanos = reading.toNanosecondsOrThrow(el::TimeEpoch::Posix);
    el::io::printLine(
        el::StringFormat{"Nanosecond alias round trip: {}; invalid split fraction accepted: {}"_el}.build(
            el::DateTime::fromNanosecondsOrThrow(nanos, el::TimeEpoch::Posix) == reading.toUtc(),
            el::DateTime::fromTicks(el::Seconds{}, el::Nanoseconds{1'000'000'000}).has_value()));
    const auto millis = reading.toTicksOrThrow<el::Milliseconds>(el::TimeEpoch::Posix);
    el::io::printLine(
        el::StringFormat{"Millisecond reconstruction: {}"_el}.build(
            el::DateTime::fromTicksOrThrow(millis, el::TimeEpoch::Posix)));
    el::io::printLine(
        el::StringFormat{
            "Modern date fits Core nanoseconds: {}; Core epoch fits POSIX seconds: {}; negative ticks accepted: {}"_el}
            .build(
                reading.toNanoseconds().has_value(),
                el::DateTime::epoch().toSeconds(el::TimeEpoch::Posix).has_value(),
                el::DateTime::fromSeconds(el::Seconds{-1}, el::TimeEpoch::Posix).has_value()));
    try {
        const auto unexpected = reading.toNanosecondsOrThrow();
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toRawValue()));
    } catch (const el::err::OutOfRangeError &) {
        el::io::printLine("Checked conversion rejects a nanosecond total that does not fit."_el);
    }
    const auto native = reading.toTimeT();
    el::io::printLine(
        el::StringFormat{"Native whole-second reconstruction: {}; pre-POSIX native output: {}"_el}.build(
            el::DateTime::fromTimeT(native), el::DateTime::first().toTimeT()));
}

}
