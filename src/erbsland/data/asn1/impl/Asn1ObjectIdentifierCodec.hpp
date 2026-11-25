// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Asn1ObjectIdentifier.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../text/String_fwd.hpp"
#include "../../../text/StringEditor_fwd.hpp"
#include "../../../unit/ByteIndex.hpp"

namespace erbsland::data::asn1::impl {

/// Decode and validate canonical X.690 OBJECT IDENTIFIER content.
/// @tested{Asn1NodeTest X509CertificateTest}
class Asn1ObjectIdentifierCodec final {
public:
    /// Create a codec for shared OBJECT IDENTIFIER content.
    Asn1ObjectIdentifierCodec(mem::ByteBlock content, unit::ByteIndex contentOffset) noexcept;

public:
    /// Decode canonical OBJECT IDENTIFIER content.
    /// @return The canonical dotted-decimal representation.
    /// @throws err::ParseError If the content is malformed or noncanonical.
    [[nodiscard]] auto decode() const -> text::String;
    /// Validate canonical OBJECT IDENTIFIER content without formatting it.
    /// @throws err::ParseError If the content is malformed or noncanonical.
    void validate() const;
    /// Append a validated object identifier as canonical X.690 content octets.
    static void encode(const Asn1ObjectIdentifier &value, mem::ByteBlockEditor &output, unit::ByteCount limit);

private:
    /// Validate all arcs and append their dotted-decimal representation when output is enabled.
    void parse(text::StringEditor *output) const;
    /// Append one decoded subidentifier when formatting is enabled.
    static void appendArc(text::StringEditor *output, uint64_t value, bool firstSubIdentifier);
    /// Append one unsigned integer using minimal base-128 encoding.
    static void appendEncodedArc(mem::ByteBlockEditor &output, uint64_t value, unit::ByteCount limit);

private:
    mem::ByteBlock _content;        ///< The shared content octets.
    unit::ByteIndex _contentOffset; ///< The source position used for diagnostics.
};

}
