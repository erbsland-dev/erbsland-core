// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1ObjectIdentifier.hpp"

#include "../../err/OverflowError.hpp"
#include "../../err/ParseError.hpp"
#include "../../text/IntegerBase.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../unit/CpLength.hpp"

#include <limits>

namespace erbsland::cryptology {

using namespace text;
using namespace unit;

auto Asn1ObjectIdentifier::fromString(const text::String &value) noexcept -> std::optional<Asn1ObjectIdentifier> {
    try {
        return fromStringOrThrow(value);
    } catch (const err::ParseError &) {
        return std::nullopt;
    } catch (const err::OverflowError &) {
        return std::nullopt;
    }
}

auto Asn1ObjectIdentifier::fromStringOrThrow(const String &value) -> Asn1ObjectIdentifier {
    auto reader = StringCharReader{value};
    auto parseOptions = IntegerParseOptions{};
    parseOptions.setFixedBase(IntegerBase::Decimal);
    uint64_t firstArc{};
    uint64_t secondArc{};
    auto arcIndex = std::size_t{};
    while (!reader.isAtEnd()) {
        const auto arcStart = reader.position();
        const auto firstCharacter = reader.peek();
        if (!firstCharacter.isAsciiDigit()) {
            throw err::ParseError{"Malformed ASN.1 object identifier."};
        }
        auto arc = uint64_t{};
        try {
            arc = reader.readIntegerOrThrow<uint64_t>(parseOptions);
        } catch (const err::OverflowError &) {
            throw err::ParseError{"ASN.1 object identifier arc is too large."};
        }
        if (firstCharacter == U'0' && reader.position() != arcStart + CpLength::one()) {
            throw err::ParseError{"ASN.1 object identifier is not canonical."};
        }
        if (arcIndex == 0U) {
            firstArc = arc;
        } else if (arcIndex == 1U) {
            secondArc = arc;
        }
        ++arcIndex;
        if (reader.isAtEnd()) {
            break;
        }
        if (!reader.readIf(U'.') || reader.isAtEnd()) {
            throw err::ParseError{"Malformed ASN.1 object identifier."};
        }
    }
    if (arcIndex < 2U || firstArc > 2U || (firstArc < 2U && secondArc > 39U) ||
        (firstArc == 2U && secondArc > std::numeric_limits<uint64_t>::max() - 80U)) {
        throw err::ParseError{"Malformed ASN.1 object identifier."};
    }
    return Asn1ObjectIdentifier{value};
}

}
