// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BerEncoder.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::data::ber {

using namespace asn1;
using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

auto BerEncoder::beginSequence() -> Scope {
    return _writer.begin(Asn1UniversalType::Sequence, true);
}

auto BerEncoder::beginSetOf() -> Scope {
    return _writer.begin(Asn1UniversalType::Set, true, asn1::impl::Asn1Writer::Order::Insertion);
}

auto BerEncoder::beginConstructed(const Asn1Tag tag) -> Scope {
    requireImplicit(tag);
    return _writer.begin(tag, true);
}

void BerEncoder::appendPrimitive(const Asn1Tag tag, const ConstByteSpan content) {
    requireImplicit(tag);
    _writer.primitive(tag, content);
}

void BerEncoder::requireImplicit(const Asn1Tag tag) {
    if (tag.tagClass != Asn1TagClass::Context && tag.tagClass != Asn1TagClass::Application) {
        throw err::ParameterFailure{"Expected an application or context-specific identifier."_el, "tag"_el};
    }
}

void BerEncoder::requireType(const Asn1Tag tag, const Asn1UniversalType type) {
    if (tag != Asn1Tag{type}) {
        requireImplicit(tag);
    }
}

void BerEncoder::appendBoolean(bool value, asn1::Asn1Tag tag) {
    requireType(tag, Asn1UniversalType::Boolean);
    auto byte = Byte{};
    if (value) {
        byte = Byte{0xffU};
    }
    _writer.primitive(tag, {&byte, 1U});
}

void BerEncoder::appendInteger(int64_t value, asn1::Asn1Tag tag) {
    requireType(tag, Asn1UniversalType::Integer);
    _writer.integer(tag, value);
}

void BerEncoder::appendEnumerated(int64_t value, asn1::Asn1Tag tag) {
    requireType(tag, Asn1UniversalType::Enumerated);
    _writer.integer(tag, value);
}

void BerEncoder::appendNull(asn1::Asn1Tag tag) {
    requireType(tag, Asn1UniversalType::Null);
    _writer.primitive(tag, {});
}

void BerEncoder::appendOctetString(mem::ConstByteSpan value, asn1::Asn1Tag tag) {
    requireType(tag, Asn1UniversalType::OctetString);
    _writer.primitive(tag, value);
}

}
