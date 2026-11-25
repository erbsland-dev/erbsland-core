// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/Asn1Limits.hpp"
#include "../asn1/Asn1Node.hpp"

namespace erbsland::data::der {

/// Decode a complete DER document.
/// @tested{Asn1NodeTest X509CertificateTest}
class DerParser final {
public:
    /// Parse one complete document.
    /// @param bytes Encoded document.
    /// @param limits Resource budgets.
    /// @return The parsed root or no value for malformed input.
    [[nodiscard]] static auto parse(const mem::ByteBlock &bytes, asn1::Asn1Limits limits = {}) noexcept
        -> std::optional<asn1::Asn1Node>;
    /// Parse one complete document with positioned diagnostics.
    /// @param bytes Encoded document.
    /// @param limits Resource budgets.
    /// @return The parsed root.
    /// @throws err::ParseError For invalid input, excess resource use or trailing bytes.
    [[nodiscard]] static auto parseOrThrow(const mem::ByteBlock &bytes, asn1::Asn1Limits limits = {}) -> asn1::Asn1Node;
    /// Parse the complete content of an enclosing node at its document-relative offset.
    /// @param node The enclosing primitive value.
    /// @param limits Resource budgets.
    /// @return The nested document root.
    /// @throws err::ParseError For empty, constructed or malformed content.
    [[nodiscard]] static auto parseContentOrThrow(const asn1::Asn1Node &node, asn1::Asn1Limits limits = {})
        -> asn1::Asn1Node;
    /// Test canonical SET component order by ASN.1 tag.
    /// @param node A constructed set or implicitly tagged set.
    [[nodiscard]] static auto isSetOrdered(const asn1::Asn1Node &node) noexcept -> bool;
    /// Test canonical SET OF order by complete encoded value.
    /// @param node A constructed set or implicitly tagged set.
    [[nodiscard]] static auto isSetOfOrdered(const asn1::Asn1Node &node) noexcept -> bool;
};

}
