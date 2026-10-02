// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Choose the precision and range of a signed distance between recorded instants.
/// @notest{Compiled and executed documentation demo.}
void distances() {
    const auto start = el::Timestamp::fromIsoStringOrThrow("2026-07-01T12:00:00.900000000Z"_el);
    const auto finish = start.addedOrThrow(el::Milliseconds{250});
    el::io::printLine(
        el::StringFormat{"Seconds: {}; milliseconds: {}; microseconds: {}; nanoseconds: {}"_el}.build(
            start.secondsToOrThrow(finish).toRawValue(),
            start.millisecondsToOrThrow(finish).toRawValue(),
            start.microsecondsToOrThrow(finish).toRawValue(),
            start.nanosecondsToOrThrow(finish).toRawValue()));
    el::io::printLine(
        el::StringFormat{"Whole duration (seconds): {}; precise delta: {}; reverse: {}; subtraction: {}"_el}.build(
            start.durationToOrThrow(finish).toSeconds().toRawValue(),
            start.timeDeltaToOrThrow(finish),
            finish.timeDeltaTo(start),
            finish - start));
    el::io::printLine(
        el::StringFormat{"Full calendar span fits seconds: {}; fits precise delta: {}"_el}.build(
            !el::Timestamp::first().wouldSecondsToSaturate(el::Timestamp::last()),
            !el::Timestamp::first().wouldTimeDeltaToSaturate(el::Timestamp::last())));
    try {
        const auto unexpected = el::Timestamp::first().timeDeltaToOrThrow(el::Timestamp::last());
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
    } catch (const el::err::OverflowError &) {
        el::io::printLine("Two valid instants can be too far apart for TimeDelta."_el);
    }
    el::io::printLine(
        el::StringFormat{"Tolerant invalid distance: {}"_el}.build(el::Timestamp{}.secondsTo(start).toRawValue()));
    try {
        const auto unexpected = el::Timestamp{}.secondsToOrThrow(start);
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toRawValue()));
    } catch (const el::err::ParameterError &) {
        el::io::printLine("Checked distance rejects an invalid operand."_el);
    }
}

}
