// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TimeDemos.hpp"

#include <erbsland/time/all.hpp>

namespace demo {

/// Compare wall-clock times and inspect fractional seconds.
/// @notest{Compiled and executed documentation demo.}
void inspect() {
    const auto reading = el::Time{el::Hour{23}, el::Minute{45}, el::Second{12}, el::Nanoseconds{123456789}};
    const auto parts = reading.parts();
    el::io::printLine(
        el::StringFormat{"Hour: {}; minute: {}; second: {}; fraction: {} ns"_el}.build(
            parts.hour.toValue(),
            parts.minute.toValue(),
            parts.second.toValue(),
            parts.nanosecondFraction.toRawValue()));
    el::io::printLine(
        el::StringFormat{"Individual fields: {} / {} / {}"_el}.build(
            reading.hour().toValue(), reading.minute().toValue(), reading.second().toValue()));
    el::io::printLine(
        el::StringFormat{"Millisecond fraction: {}; nanosecond fraction: {}"_el}.build(
            reading.millisecondFraction().toRawValue(), reading.nanosecondFraction().toRawValue()));
    el::io::printLine(
        el::StringFormat{"Midnight is zero: {}; reading is zero: {}"_el}.build(el::Time{}.isZero(), reading.isZero()));
    el::io::printLine(
        el::StringFormat{"Midnight sorts before evening: {}; same time equals: {}"_el}.build(
            el::Time{} < reading,
            reading == el::Time{parts.hour, parts.minute, parts.second, parts.nanosecondFraction}));
}

}
