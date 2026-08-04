// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1ObjectIdentifierCodec.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/OverflowError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteIntegerFormat.hpp"
#include "../../mem/ByteReader.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ByteIndex.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

Asn1ObjectIdentifierCodec::Asn1ObjectIdentifierCodec(
    const mem::ByteBlock &content, const ByteIndex contentOffset) noexcept :
    _content{content}, _contentOffset{contentOffset} {
}

auto Asn1ObjectIdentifierCodec::decode() const -> String {
    auto result = StringEditor{};
    parse(&result);
    return String{result};
}

void Asn1ObjectIdentifierCodec::validate() const {
    parse(nullptr);
}

void Asn1ObjectIdentifierCodec::parse(StringEditor *const output) const {
    // ITU-T X.690 section 8.19: OID subidentifiers are minimal unsigned base-128 integers.
    if (_content.isEmpty()) {
        throw err::ParseError{"DER OBJECT IDENTIFIER is empty."_el, _contentOffset};
    }
    auto reader = mem::ByteReader{_content};
    auto firstSubIdentifier = true;
    while (!reader.isAtEnd()) {
        const auto relativeOffset = reader.position();
        const auto arcOffset = _contentOffset + relativeOffset.distanceFromZero();
        if ((reader.peekByte().toUInt8() & 0x7fU) == 0U && reader.peekByte().toUInt8() != 0U) {
            throw err::ParseError{"DER OBJECT IDENTIFIER arc is not minimally encoded."_el, arcOffset};
        }
        auto value = uint64_t{};
        try {
            value = reader.readIntegerOrThrow<uint64_t>(mem::ByteIntegerFormat::UnsignedBase128);
        } catch (const err::OutOfRangeError &) {
            throw err::ParseError{"DER OBJECT IDENTIFIER is truncated."_el, arcOffset};
        } catch (const err::OverflowError &) {
            throw err::ParseError{"DER OBJECT IDENTIFIER arc is too large."_el, arcOffset};
        } catch (const err::ParseError &) {
            throw err::ParseError{"DER OBJECT IDENTIFIER arc is not minimally encoded."_el, arcOffset};
        }
        appendArc(output, value, firstSubIdentifier);
        firstSubIdentifier = false;
    }
}

void Asn1ObjectIdentifierCodec::appendArc(
    StringEditor *const output, const uint64_t value, const bool firstSubIdentifier) {
    if (output == nullptr) {
        return;
    }
    if (firstSubIdentifier) {
        const auto first = value < 40U ? uint64_t{} : (value < 80U ? uint64_t{1U} : uint64_t{2U});
        output->append(String::fromInteger(first));
        output->append(U'.');
        output->append(String::fromInteger(value - first * 40U));
        return;
    }
    output->append(U'.');
    output->append(String::fromInteger(value));
}

}
