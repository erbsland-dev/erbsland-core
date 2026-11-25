// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BerParser.hpp"

#include "../asn1/impl/Asn1Decoder.hpp"

#include "../../err/ParseError.hpp"

namespace erbsland::data::ber {

using asn1::impl::Asn1Decoder;

auto BerParser::parse(const mem::ByteBlock &bytes, const asn1::Asn1Limits limits) noexcept
    -> std::optional<asn1::Asn1Node> {
    try {
        return parseOrThrow(bytes, limits);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto BerParser::parseOrThrow(const mem::ByteBlock &bytes, const asn1::Asn1Limits limits) -> asn1::Asn1Node {
    return Asn1Decoder{Asn1Decoder::EncodingRules::LdapBer, limits}.parse(bytes);
}

auto BerParser::frameLength(const mem::ByteBlock &bytes, const asn1::Asn1Limits limits)
    -> std::optional<unit::ByteCount> {
    return Asn1Decoder{Asn1Decoder::EncodingRules::LdapBer, limits}.frameLength(bytes);
}

}
