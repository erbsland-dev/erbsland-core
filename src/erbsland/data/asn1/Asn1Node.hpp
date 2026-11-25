// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Asn1BitString.hpp"
#include "Asn1Node_fwd.hpp"
#include "Asn1ObjectIdentifier.hpp"
#include "Asn1Tag.hpp"
#include "Asn1TagClass.hpp"
#include "Asn1UniversalType.hpp"

#include "impl/DecodedNode_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/List.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace erbsland::data::asn1 {

/// An immutable parsed value sharing its encoded bytes and children.
/// @seedoc{/reference/data/asn1}
/// @tested{Asn1NodeTest BerCodecTest X509CertificateTest}
class Asn1Node final {
public:
    /// Create an empty node.
    Asn1Node() = default;

    /// Take ownership of a decoder-validated value.
    /// @param value The validated node representation.
    explicit Asn1Node(impl::DecodedNode &&value) noexcept;

    // defaults
    ~Asn1Node() = default;
    Asn1Node(const Asn1Node &) = default;
    Asn1Node(Asn1Node &&) noexcept = default;
    auto operator=(const Asn1Node &) -> Asn1Node & = default;
    auto operator=(Asn1Node &&) noexcept -> Asn1Node & = default;

public: // tests
    /// Test if this node is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this node is constructed.
    [[nodiscard]] auto isConstructed() const noexcept -> bool;

    /// Test the exact tag and form.
    /// @param expected Expected identifier class and number.
    /// @param constructed Expected constructed flag.
    [[nodiscard]] auto matches(Asn1Tag expected, bool constructed) const noexcept -> bool;
    /// Test whether this is a primitive NULL with empty content.
    /// @param expected Expected universal or implicit tag.
    [[nodiscard]] auto isNull(Asn1Tag expected = Asn1UniversalType::Null) const noexcept -> bool;
    /// Test whether this value was parsed under the DER rules.
    [[nodiscard]] auto isDer() const noexcept -> bool { return _der && !isEmpty(); }

public: // accessors
    /// Get the identifier.
    [[nodiscard]] auto tag() const noexcept -> Asn1Tag { return _tag; }
    /// Get the document-relative position of the identifier.
    [[nodiscard]] auto encodedByteIndex() const noexcept -> unit::ByteIndex { return _offset; }
    /// Get the document-relative position of the first content byte.
    [[nodiscard]] auto contentByteIndex() const noexcept -> unit::ByteIndex { return _offset + _headerLength; }

    /// Get the encoded tag class.
    [[nodiscard]] auto tagClass() const noexcept -> Asn1TagClass;
    /// Get the numeric tag.
    [[nodiscard]] auto tagNumber() const noexcept -> uint32_t;
    /// Get the universal type, or `None` for a non-universal node.
    [[nodiscard]] auto universalType() const noexcept -> Asn1UniversalType;
    /// Get the number of direct child nodes.
    [[nodiscard]] auto childCount() const noexcept -> unit::ItemCount;
    /// Get a child node or an empty node for an invalid index.
    [[nodiscard]] auto child(unit::ItemIndex index) const noexcept -> Asn1Node;
    /// Get all direct child nodes.
    [[nodiscard]] auto children() const -> util::List<Asn1Node>;

public: // conversion
    /// Return the exact original encoding of this node.
    [[nodiscard]] auto encodedData() const -> mem::ByteBlock;
    /// Return only the content octets of this node.
    [[nodiscard]] auto contentData() const -> mem::ByteBlock;
    /// Decode a BOOLEAN value, including an implicitly tagged primitive.
    /// @param expected Expected wire identifier.
    /// @return The decoded value if the tag, form, content and native range match.
    [[nodiscard]] auto toBoolean(Asn1Tag expected = Asn1UniversalType::Boolean) const noexcept -> std::optional<bool>;
    /// Decode an OBJECT IDENTIFIER value, including an implicitly tagged primitive.
    /// @param expected Expected wire identifier.
    /// @return The decoded value if the tag, form, content and native range match.
    [[nodiscard]] auto toObjectIdentifier(Asn1Tag expected = Asn1UniversalType::ObjectIdentifier) const noexcept
        -> std::optional<Asn1ObjectIdentifier>;
    /// Decode an object identifier with a positioned diagnostic.
    /// @param expected Expected primitive identifier.
    /// @return The decoded identifier.
    /// @throws err::ParseError If the tag or OID content is invalid.
    [[nodiscard]] auto toObjectIdentifierOrThrow(Asn1Tag expected = Asn1UniversalType::ObjectIdentifier) const
        -> Asn1ObjectIdentifier;
    /// Decode an INTEGER value, including an implicitly tagged primitive.
    /// @param expected Expected wire identifier.
    /// @return The decoded value if the tag, form, content and native range match.
    [[nodiscard]] auto toInteger(Asn1Tag expected = Asn1UniversalType::Integer) const noexcept
        -> std::optional<int64_t>;
    /// Decode a nonnegative INTEGER as an unsigned native value.
    /// @param expected Expected universal or implicit identifier.
    /// @return The decoded value if the tag, form, content and unsigned range match.
    [[nodiscard]] auto toUnsignedInteger(Asn1Tag expected = Asn1UniversalType::Integer) const noexcept
        -> std::optional<uint64_t>;
    /// Get the shared big-endian magnitude of a nonnegative integer.
    /// @param expected Expected primitive identifier.
    /// @return The magnitude without sign padding, an empty block for zero, or no value for an invalid integer.
    [[nodiscard]] auto toUnsignedMagnitude(Asn1Tag expected = Asn1UniversalType::Integer) const noexcept
        -> std::optional<mem::ByteBlock>;
    /// Decode an ENUMERATED value, including an implicitly tagged primitive.
    /// @param expected Expected wire identifier.
    /// @return The decoded value if the tag, form, content and native range match.
    [[nodiscard]] auto toEnumerated(Asn1Tag expected = Asn1UniversalType::Enumerated) const noexcept
        -> std::optional<int64_t>;
    /// Decode a BIT STRING value, including an implicitly tagged primitive.
    /// @param expected Expected wire identifier.
    /// @return The decoded value if the tag, form, content and native range match.
    [[nodiscard]] auto toBitString(Asn1Tag expected = Asn1UniversalType::BitString) const noexcept
        -> std::optional<Asn1BitString>;
    /// Decode an OCTET STRING value, including an implicitly tagged primitive.
    /// @param expected Expected wire identifier.
    /// @return The decoded value if the tag, form, content and native range match.
    [[nodiscard]] auto toOctetString(Asn1Tag expected = Asn1UniversalType::OctetString) const noexcept
        -> std::optional<mem::ByteBlock>;
    /// Decode a supported ASN.1 character string.
    [[nodiscard]] auto toString() const noexcept -> std::optional<text::String>;

private:
    /// Check the primitive form and the expected universal or implicit tag.
    [[nodiscard]] auto matchesPrimitive(Asn1Tag expected, Asn1UniversalType type) const noexcept -> bool;
    /// Decode the signed content of an INTEGER or ENUMERATED value.
    [[nodiscard]] auto signedInteger(Asn1Tag expected, Asn1UniversalType type) const noexcept -> std::optional<int64_t>;

private:
    mem::ByteBlock _encodedData;           ///< Original encoding.
    unit::ByteIndex _offset;               ///< Document-relative identifier position.
    unit::ByteCount _headerLength;         ///< Encoded header length.
    Asn1Tag _tag{Asn1UniversalType::None}; ///< Decoded tag.
    bool _constructed{};                   ///< Constructed form.
    bool _der{};                           ///< Validated under DER rules.
    util::List<Asn1Node> _children;        ///< Direct children.
};

}
