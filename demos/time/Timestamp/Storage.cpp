// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Store canonical timestamp fields or a portable twelve-byte representation.
/// @notest{Compiled and executed documentation demo.}
void storage() {
    const auto event = el::Timestamp::fromIsoStringOrThrow("2026-07-01T12:30:00.123456789Z"_el);
    const auto raw = event.toRawValue();
    el::io::printLine(
        el::StringFormat{"Raw days: {}; nanoseconds: {}; restored: {}"_el}.build(
            raw.first, raw.second, el::Timestamp::fromRawValueOrThrow(raw) == event));
    el::io::printLine(
        el::StringFormat{"Separate fields restored: {}; invalid fields canonicalized: {}"_el}.build(
            el::Timestamp::fromRawValueOrThrow(raw.first, raw.second) == event,
            el::Timestamp::fromRawValueOrThrow(-4, 27) == el::Timestamp{}));
    const auto bytes = event.toByteBlock();
    el::io::printLine(
        el::StringFormat{"Encoded: {}; byte round trip: {}"_el}.build(
            el::String::fromByteBlock(bytes), el::Timestamp::fromByteBlockOrThrow(bytes) == event));
    el::io::printLine(
        el::StringFormat{"Invalid encoding: {}; empty block accepted: {}"_el}.build(
            el::String::fromByteBlock(el::Timestamp{}.toByteBlock()),
            el::Timestamp::fromByteBlock(el::ByteBlock{}).has_value()));
    try {
        const auto unexpected = el::Timestamp::fromByteBlockOrThrow(el::ByteBlock{});
        el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
    } catch (const el::err::ParameterError &) {
        el::io::printLine("Checked decoding rejects the wrong byte count."_el);
    }
}

}
