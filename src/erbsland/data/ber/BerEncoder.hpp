// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/impl/Asn1Writer.hpp"

namespace erbsland::data::ber {

/// Stateful encoder for LDAP-profile BER values.
/// @tested{BerCodecTest}
class BerEncoder final {
public:
    /// Handle for an open value in this encoder.
    using Scope = asn1::impl::Asn1Writer::Scope;
    /// Create an empty encoder.
    /// @param limits Output resource budgets.
    explicit BerEncoder(asn1::Asn1Limits limits = {}) noexcept : _writer{limits} {}

public: // output
    /// Mark the output allocation as sensitive.
    void markAsSensitive() noexcept { _writer.markAsSensitive(); }
    /// Get the encoded values. All scopes must be closed.
    [[nodiscard]] auto encoded() const -> mem::ByteBlock { return _writer.encoded(); }
    /// Close the most recently opened value.
    /// @param scope The handle from this encoder.
    void end(Scope scope) { _writer.end(scope); }
    /// Discard the most recently opened value and its children.
    /// @param scope The handle from this encoder.
    void cancel(Scope scope) { _writer.cancel(scope); }

public: // containers
    /// Begin a SEQUENCE.
    [[nodiscard]] auto beginSequence() -> Scope;
    /// Begin a SET OF with children in insertion order.
    [[nodiscard]] auto beginSetOf() -> Scope;
    /// Begin a constructed application or context-specific value.
    /// @param tag Wire identifier.
    [[nodiscard]] auto beginConstructed(asn1::Asn1Tag tag) -> Scope;
    /// Append a primitive application or context-specific value.
    /// @param tag Wire identifier.
    /// @param content Opaque primitive content.
    void appendPrimitive(asn1::Asn1Tag tag, mem::ConstByteSpan content);

public: // primitive values
    /// Append a BOOLEAN value.
    /// @param value Value to encode.
    /// @param tag Universal or implicit wire identifier.
    void appendBoolean(bool value, asn1::Asn1Tag tag = asn1::Asn1UniversalType::Boolean);
    /// Append an INTEGER value.
    /// @param value Value to encode.
    /// @param tag Universal or implicit wire identifier.
    void appendInteger(int64_t value, asn1::Asn1Tag tag = asn1::Asn1UniversalType::Integer);
    /// Append an ENUMERATED value.
    /// @param value Value to encode.
    /// @param tag Universal or implicit wire identifier.
    void appendEnumerated(int64_t value, asn1::Asn1Tag tag = asn1::Asn1UniversalType::Enumerated);
    /// Append a NULL value.
    /// @param tag Universal or implicit wire identifier.
    void appendNull(asn1::Asn1Tag tag = asn1::Asn1UniversalType::Null);
    /// Append an OCTET STRING value.
    /// @param value Value to encode.
    /// @param tag Universal or implicit wire identifier.
    void appendOctetString(mem::ConstByteSpan value, asn1::Asn1Tag tag = asn1::Asn1UniversalType::OctetString);

private:
    /// Require a protocol-defined tag for a schema-supplied primitive or container.
    static void requireImplicit(asn1::Asn1Tag tag);
    /// Accept either the expected universal type or an implicit protocol tag.
    static void requireType(asn1::Asn1Tag tag, asn1::Asn1UniversalType type);
    asn1::impl::Asn1Writer _writer; ///< Bounded output and scope state.
};

}
