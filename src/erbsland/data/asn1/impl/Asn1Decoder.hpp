// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Asn1Limits.hpp"
#include "../Asn1Node.hpp"

#include "../../../mem/ByteReader.hpp"

namespace erbsland::data::asn1::impl {

/// Bounded definite-length decoder sharing its header reader with the framing probe.
/// @tested{Asn1NodeTest BerCodecTest X509CertificateTest}
class Asn1Decoder final {
    /// A complete decoded identifier and definite length.
    struct Header {
        Asn1Tag tag;            ///< Identifier class and number.
        bool constructed;       ///< Constructed encoding.
        unit::ByteCount length; ///< Content length.
    };

public:
    /// Supported definite-length wire profiles.
    enum class EncodingRules {
        Der,     ///< Distinguished encoding rules.
        LdapBer, ///< LDAP protocol encoding under RFC 4511.
    };
    /// Create a decoder for one document.
    /// @param rules Wire profile.
    /// @param limits Per-document resource budgets.
    Asn1Decoder(EncodingRules rules, Asn1Limits limits) noexcept : _rules{rules}, _limits{limits} {}
    /// Decode a complete value and retain its original source coordinates.
    /// @param bytes Document bytes.
    /// @param base Parent content position for an embedded document.
    /// @return The decoded root.
    /// @throws err::ParseError On invalid input.
    [[nodiscard]] auto parse(const mem::ByteBlock &bytes, unit::ByteIndex base = {}) -> Asn1Node;
    /// Determine the length of the first complete frame.
    /// @param bytes Available receive buffer.
    /// @return The frame length or no value for an incomplete frame.
    /// @throws err::ParseError For an invalid header or excessive length.
    [[nodiscard]] auto frameLength(const mem::ByteBlock &bytes) const -> std::optional<unit::ByteCount>;

private:
    /// Read one header or return no value for an incomplete prefix.
    [[nodiscard]] auto readHeader(mem::ByteReader &reader, unit::ByteIndex base) const -> std::optional<Header>;
    /// Read one child inside the bounded parent reader.
    [[nodiscard]] auto parseNode(
        mem::ByteReader &reader, const mem::ByteBlock &source, unit::ByteIndex base, unit::ItemCount depth) -> Asn1Node;
    /// Validate the identifier and encoding form for the selected profile.
    void validateForm(Asn1Tag tag, bool constructed, unit::ByteIndex offset) const;
    /// Validate a supported universal primitive at its document offset.
    static void validatePrimitive(Asn1UniversalType type, const mem::ByteBlock &bytes, unit::ByteIndex contentOffset);
    /// Check each character against the X.680 PrintableString repertoire.
    static void validatePrintableString(mem::ConstByteSpan content, unit::ByteIndex contentOffset);
    /// Report a malformed document at the given byte position.
    [[noreturn]] static void throwParseError(text::String reason, unit::ByteIndex byteIndex);
    EncodingRules _rules;   ///< Wire profile.
    Asn1Limits _limits;     ///< Document budgets.
    unit::ItemCount _nodes; ///< Nodes decoded so far.
};

}
