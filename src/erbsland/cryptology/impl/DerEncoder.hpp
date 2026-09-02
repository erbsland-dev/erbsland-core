// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/Asn1ObjectIdentifier.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/String_fwd.hpp"
#include "../../time/DateTime_fwd.hpp"
#include "../../util/List_fwd.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// A bounded stateful encoder that appends canonical DER to one final byte container.
/// @tested{DerEncoderTest SigningPrivateKeyTest}
class DerEncoder final {
public:
    /// A token for one open constructed or explicitly wrapped value.
    /// @tested{DerEncoderTest}
    class Scope final {
        friend class DerEncoder;

    private:
        /// Create a scope token for one open value.
        /// @param lengthIndex Start of the reserved length field.
        /// @param contentIndex Start of the value content.
        /// @param depth Nesting depth after opening the value.
        Scope(std::size_t lengthIndex, std::size_t contentIndex, std::size_t depth) noexcept :
            _lengthIndex{lengthIndex}, _contentIndex{contentIndex}, _depth{depth} {}

    private:
        std::size_t _lengthIndex{};  ///< Start of the reserved length field.
        std::size_t _contentIndex{}; ///< Start of the encoded content.
        std::size_t _depth{};        ///< Nesting depth used to enforce LIFO closure.
    };

public:
    /// Create an empty encoder.
    DerEncoder() = default;

    // defaults
    ~DerEncoder() = default;
    DerEncoder(const DerEncoder &) = delete;
    DerEncoder(DerEncoder &&) noexcept = default;
    auto operator=(const DerEncoder &) -> DerEncoder & = delete;
    auto operator=(DerEncoder &&) noexcept -> DerEncoder & = default;

public: // state
    /// Mark the final allocation as sensitive.
    void markAsSensitive() noexcept;
    /// Append an already complete canonical DER value.
    void appendEncoded(mem::ConstByteSpan value);
    /// Return the complete encoding. All scopes must be closed.
    [[nodiscard]] auto encoded() const -> mem::ByteBlock;

public: // constructed and wrapped values
    /// Begin a SEQUENCE.
    [[nodiscard]] auto beginSequence() -> Scope;
    /// Begin a SET whose caller supplies canonically ordered children.
    [[nodiscard]] auto beginSet() -> Scope;
    /// Begin a constructed context-specific value.
    [[nodiscard]] auto beginExplicit(uint8_t tagNumber) -> Scope;
    /// Begin an OCTET STRING whose content is appended directly.
    [[nodiscard]] auto beginOctetString() -> Scope;
    /// Begin a BIT STRING containing complete octets.
    [[nodiscard]] auto beginBitString() -> Scope;
    /// Close the most recently opened scope and backpatch its canonical length.
    void end(Scope scope);
    /// Append a canonically sorted SET from independently encoded child values.
    void appendSet(util::List<mem::ByteBlock> children);

public: // primitive values
    /// Append a BOOLEAN.
    void appendBoolean(bool value);
    /// Append a nonnegative native INTEGER.
    void appendPositiveInteger(uint64_t value);
    /// Append a nonnegative INTEGER from a big-endian magnitude.
    void appendPositiveInteger(mem::ConstByteSpan magnitude);
    /// Append NULL.
    void appendNull();
    /// Append an OBJECT IDENTIFIER.
    void appendObjectIdentifier(const Asn1ObjectIdentifier &value);
    /// Append a UTF8String.
    void appendUtf8String(const text::String &value);
    /// Append a validated PrintableString.
    void appendPrintableString(const text::String &value);
    /// Append a validated IA5String.
    void appendIa5String(const text::String &value);
    /// Append an OCTET STRING.
    void appendOctetString(mem::ConstByteSpan content);
    /// Append a BIT STRING.
    void appendBitString(mem::ConstByteSpan content, uint8_t unusedBits = 0U);
    /// Append an RFC 5280 Time value.
    void appendTime(const time::DateTime &value);
    /// Append a primitive context-specific value.
    void appendImplicitPrimitive(uint8_t tagNumber, mem::ConstByteSpan content);

private:
    /// Begin an arbitrary low-tag-number value with reserved length storage.
    [[nodiscard]] auto beginValue(uint8_t tag) -> Scope;
    /// Append a complete primitive value.
    void appendValue(uint8_t tag, mem::ConstByteSpan content);
    /// Validate BIT STRING content and append its first content octet.
    void validateBitString(mem::ConstByteSpan content, uint8_t unusedBits) const;

private:
    mem::ByteBlockEditor _data; ///< The one final output allocation.
    std::size_t _depth{};       ///< Number of currently open scopes.
};

}
