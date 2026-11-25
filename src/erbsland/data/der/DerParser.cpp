// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DerParser.hpp"

#include "../asn1/impl/Asn1Decoder.hpp"

#include "../../err/ParseError.hpp"

namespace erbsland::data::der {

using asn1::impl::Asn1Decoder;

auto DerParser::parse(const mem::ByteBlock &bytes, const asn1::Asn1Limits limits) noexcept
    -> std::optional<asn1::Asn1Node> {
    try {
        return parseOrThrow(bytes, limits);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto DerParser::parseOrThrow(const mem::ByteBlock &bytes, const asn1::Asn1Limits limits) -> asn1::Asn1Node {
    return Asn1Decoder{Asn1Decoder::EncodingRules::Der, limits}.parse(bytes);
}

auto DerParser::parseContentOrThrow(const asn1::Asn1Node &node, const asn1::Asn1Limits limits) -> asn1::Asn1Node {
    if (node.isEmpty() || node.isConstructed()) {
        throw err::ParseError{"Expected a primitive containing DER.", node.encodedByteIndex()};
    }
    return Asn1Decoder{Asn1Decoder::EncodingRules::Der, limits}.parse(node.contentData(), node.contentByteIndex());
}

auto DerParser::isSetOrdered(const asn1::Asn1Node &node) noexcept -> bool {
    if (!node.isConstructed() ||
        (node.tagClass() == asn1::Asn1TagClass::Universal && node.universalType() != asn1::Asn1UniversalType::Set)) {
        return false;
    }
    const auto children = node.children();
    for (auto i = unit::ItemIndex::one(); i.isWithin(children.count()); i.uncheckedIncrement()) {
        const auto &previous = children.getRefOrThrow(i.decremented());
        const auto &current = children.getRefOrThrow(i);
        if (previous.tag() >= current.tag()) {
            return false;
        }
    }
    return true;
}

auto DerParser::isSetOfOrdered(const asn1::Asn1Node &node) noexcept -> bool {
    if (!node.isConstructed() ||
        (node.tagClass() == asn1::Asn1TagClass::Universal && node.universalType() != asn1::Asn1UniversalType::Set)) {
        return false;
    }
    const auto children = node.children();
    for (auto i = unit::ItemIndex::one(); i.isWithin(children.count()); i.uncheckedIncrement()) {
        const auto &previous = children.getRefOrThrow(i.decremented());
        const auto &current = children.getRefOrThrow(i);
        if (previous.encodedData() > current.encodedData()) {
            return false;
        }
    }
    return true;
}

}
