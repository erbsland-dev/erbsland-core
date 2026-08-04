// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DerParser_fwd.hpp"
#include "X509Parser_fwd.hpp"

#include "../asn1/Asn1Node.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteReader.hpp"
#include "../../unit/ByteLength.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// Bounded canonical DER parser used by certificate input.
/// @tested{Asn1NodeTest X509CertificateTest}
class DerParser final {
    friend class X509Parser;

public:
    /// Maximum supported DER document length.
    static constexpr auto cMaximumLength = unit::ByteLength{1U * 1024U * 1024U};
    /// Maximum supported nesting depth.
    static constexpr auto cMaximumDepth = std::size_t{32U};
    /// Maximum number of ASN.1 nodes.
    static constexpr auto cMaximumNodes = unit::ItemCount{8'192U};

public:
    /// Create a new DER parser instance.
    explicit DerParser(mem::ByteBlock der) noexcept;
    /// Parse one complete canonical DER value.
    /// @return The root of the immutable parsed tree.
    /// @throws err::ParseError If the input is malformed, noncanonical, or has trailing data.
    /// @throws err::OutOfRangeError If a fixed parser resource limit is exceeded.
    [[nodiscard]] auto parseDocument() -> Asn1Node;

private:
    /// Parse one DER node and its descendants.
    [[nodiscard]] auto parseNode(std::size_t depth) -> Asn1Node;
    /// Decode the low-tag-number field or following high-tag-number integer.
    [[nodiscard]] auto readTagNumber(uint8_t identifier) -> uint32_t;
    /// Decode one canonical definite DER length.
    [[nodiscard]] auto readLength() -> unit::ByteLength;
    /// Read one byte or report a positioned truncation error.
    [[nodiscard]] auto readByte() -> uint8_t;
    /// Validate the primitive/constructed form required for a universal tag.
    void validateUniversalForm(
        Asn1TagClass tagClass, uint32_t tagNumber, bool constructed, unit::ByteIndex encodedOffset) const;
    /// Validate a supported primitive value.
    void validatePrimitive(
        Asn1TagClass tagClass, uint32_t tagNumber, unit::ByteIndex contentOffset, unit::ByteLength contentLength) const;
    /// Validate canonical DER INTEGER content.
    void validateInteger(mem::ConstByteSpan content, unit::ByteIndex contentOffset) const;
    /// Validate canonical DER BIT STRING content.
    void validateBitString(mem::ConstByteSpan content, unit::ByteIndex contentOffset) const;
    /// Validate the X.680 PrintableString repertoire.
    void validatePrintableString(mem::ConstByteSpan content, unit::ByteIndex contentOffset) const;
    /// Validate canonical DER SET ordering.
    void validateSetOrder(const util::List<Asn1Node> &children) const;
    /// Throw a parse error carrying a byte index.
    [[noreturn]] static void throwParseError(text::String reason, unit::ByteIndex byteIndex);

private:
    mem::ByteBlock _der;        ///< Immutable DER bytes retained by parsed nodes.
    mem::ByteReader _reader;    ///< Transactional sequential reader over `_der`.
    unit::ItemCount _nodeCount; ///< Number of parsed nodes.
};

}
