// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Exchange timestamp text with an explicit UTC offset and full fractional precision.
/// @notest{Compiled and executed documentation demo.}
void text() {
    const auto event = el::Timestamp::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+02:00"_el);
    el::io::printLine(
        el::StringFormat{"Canonical UTC: {}; ISO round trip: {}"_el}.build(
            event.toIsoStringOrThrow(), el::Timestamp::fromIsoStringOrThrow(event.toString()) == event));
    el::io::printLine(
        el::StringFormat{"Missing offset accepted: {}; invalid ISO available: {}"_el}.build(
            el::Timestamp::fromIsoString("2026-07-01T14:30:00"_el).has_value(),
            el::Timestamp{}.toIsoString().has_value()));
    el::io::printLine(
        el::StringFormat{"Out-of-calendar UTC accepted: {}"_el}.build(
            el::Timestamp::fromIsoString("0000-01-01T00:00:00+01:00"_el).has_value()));
    try {
        const auto unexpected = el::Timestamp::fromIsoStringOrThrow("2026-02-30T12:00:00Z"_el);
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
    } catch (const el::err::ParseError &) {
        el::io::printLine("Checked parsing rejects an impossible date."_el);
    }
}

}
