// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1ObjectIdentifier.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/StringCharReader.hpp"

#include <limits>

namespace erbsland::cryptology {

auto Asn1ObjectIdentifier::fromString(const text::String &value) noexcept -> std::optional<Asn1ObjectIdentifier> {
    try {
        return fromStringOrThrow(value);
    } catch (...) {
        return std::nullopt;
    }
}

auto Asn1ObjectIdentifier::fromStringOrThrow(const text::String &value) -> Asn1ObjectIdentifier {
    auto reader = text::StringCharReader{value};
    uint64_t firstArc{};
    uint64_t secondArc{};
    auto arcIndex = std::size_t{};
    while (!reader.isAtEnd()) {
        const auto firstCharacter = reader.peek();
        if (!firstCharacter.isAsciiDigit()) {
            throw err::ParseError{"Malformed ASN.1 object identifier."};
        }
        auto arc = uint64_t{};
        auto digitCount = std::size_t{};
        while (!reader.isAtEnd() && reader.peek().isAsciiDigit()) {
            const auto digit = static_cast<uint64_t>(reader.read().digitValue().value());
            if (arc > (std::numeric_limits<uint64_t>::max() - digit) / 10U) {
                throw err::ParseError{"ASN.1 object identifier arc is too large."};
            }
            arc = arc * 10U + digit;
            ++digitCount;
        }
        if (digitCount > 1U && firstCharacter == U'0') {
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
    if (arcIndex < 2U || firstArc > 2U || (firstArc < 2U && secondArc > 39U)) {
        throw err::ParseError{"Malformed ASN.1 object identifier."};
    }
    return Asn1ObjectIdentifier{value};
}

}
