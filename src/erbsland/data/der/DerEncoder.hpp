// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/impl/Asn1Writer.hpp"

#include "../../time/Timestamp_fwd.hpp"

namespace erbsland::data::der {

/// Stateful encoder for DER values.
/// @tested{DerEncoderTest SigningPrivateKeyTest}
class DerEncoder final {
public:
    /// Handle for an open value in this encoder.
    using Scope = asn1::impl::Asn1Writer::Scope;
    /// Create an empty encoder.
    /// @param limits Output resource budgets.
    explicit DerEncoder(asn1::Asn1Limits limits = {}) noexcept : _writer{limits} {}

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
    /// Begin a SET OF with children in encoded byte order.
    [[nodiscard]] auto beginSetOf() -> Scope;
    /// Begin a SET with components in ascending tag order.
    [[nodiscard]] auto beginSet() -> Scope;
    /// Sort complete DER values and append them as a SET OF.
    /// @param children Independently encoded elements.
    /// @throws err::ParseError For malformed elements.
    void appendSetOf(util::List<mem::ByteBlock> children);
    /// Begin an explicitly tagged context-specific container.
    /// @param number Context-specific tag number.
    [[nodiscard]] auto beginExplicit(uint32_t number) -> Scope;
    /// Begin a primitive OCTET STRING wrapping encoded values.
    [[nodiscard]] auto beginOctetString() -> Scope;
    /// Begin an octet-aligned BIT STRING wrapping encoded values.
    [[nodiscard]] auto beginBitString() -> Scope;
    /// Append one DER-validated value.
    /// @param node A node obtained from the DER parser.
    void appendEncoded(const asn1::Asn1Node &node);
    /// Validate and append one complete DER value.
    /// @param bytes Encoded value.
    /// @throws err::ParseError For invalid DER.
    void appendEncoded(const mem::ByteBlock &bytes);

public: // primitive values
    /// Append a BOOLEAN value.
    /// @param value Value to encode.
    void appendBoolean(bool value);
    /// Append an INTEGER value.
    /// @param value Value to encode.
    void appendInteger(int64_t value);
    /// Append an ENUMERATED value.
    /// @param value Value to encode.
    void appendEnumerated(int64_t value);
    /// Append a NULL value.
    void appendNull();
    /// Append an OCTET STRING value.
    /// @param value Value to encode.
    void appendOctetString(mem::ConstByteSpan value);
    /// Append a nonnegative integer.
    /// @param value Unsigned value.
    void appendPositiveInteger(uint64_t value);
    /// Append an arbitrary-size nonnegative integer.
    /// @param magnitude Big-endian magnitude.
    void appendPositiveInteger(mem::ConstByteSpan magnitude);
    /// Append an OID.
    /// @param value Nonempty object identifier.
    void appendObjectIdentifier(const asn1::Asn1ObjectIdentifier &value);
    /// Append a UTF8String.
    /// @param value Valid UTF-8 text.
    void appendUtf8String(const text::String &value);
    /// Append a PrintableString.
    /// @param value Text in the X.680 repertoire.
    void appendPrintableString(const text::String &value);
    /// Append an IA5String.
    /// @param value ASCII text.
    void appendIa5String(const text::String &value);
    /// Append a BIT STRING.
    /// @param content Value octets.
    /// @param unusedBits Number of unused low bits in the last octet.
    void appendBitString(mem::ConstByteSpan content, uint8_t unusedBits = 0U);
    /// Append an RFC 5280 time in UTC.
    /// @param value Valid timestamp.
    void appendTime(const time::Timestamp &value);
    /// Append an implicitly tagged primitive.
    /// @param tagNumber Context-specific identifier.
    /// @param content Primitive content octets.
    void appendImplicitPrimitive(uint32_t tagNumber, mem::ConstByteSpan content);

private:
    asn1::impl::Asn1Writer _writer; ///< Bounded output and scope state.
};

}
