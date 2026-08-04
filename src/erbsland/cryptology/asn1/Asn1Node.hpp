// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Asn1ObjectIdentifier.hpp"
#include "Asn1TagClass.hpp"
#include "Asn1UniversalType.hpp"

#include "../impl/DerParser_fwd.hpp"
#include "../impl/X509Parser_fwd.hpp"
#include "../keys/PublicKey_fwd.hpp"
#include "../x509/X509AlgorithmIdentifier_fwd.hpp"
#include "../x509/X509Certificate_fwd.hpp"
#include "../x509/X509Extension_fwd.hpp"
#include "../x509/X509Name_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/List.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace erbsland::cryptology {

/// An immutable node in a certificate-owned ASN.1 tree.
/// Node values retain their shared DER storage. No factory for arbitrary ASN.1 input is exposed; obtain a node from a
/// successfully parsed certificate or one of its typed values.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{Asn1NodeTest X509CertificateTest}
class Asn1Node final {
    friend class PublicKey;
    friend class X509AlgorithmIdentifier;
    friend class X509Certificate;
    friend class X509Extension;
    friend class X509Name;
    friend class impl::DerParser;
    friend class impl::X509Parser;

public:
    /// Create an empty node.
    Asn1Node() = default;

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

public: // accessors
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
    /// Return the complete canonical DER encoding of this node.
    [[nodiscard]] auto encodedData() const -> mem::ByteBlock;
    /// Return only the content octets of this node.
    [[nodiscard]] auto contentData() const -> mem::ByteBlock;
    /// Decode a BOOLEAN value.
    [[nodiscard]] auto toBoolean() const noexcept -> std::optional<bool>;
    /// Decode an OBJECT IDENTIFIER value.
    [[nodiscard]] auto toObjectIdentifier() const noexcept -> std::optional<Asn1ObjectIdentifier>;
    /// Decode a supported ASN.1 character string.
    [[nodiscard]] auto toString() const noexcept -> std::optional<text::String>;

private:
    /// Create an ASN.1 node from its encoded representation.
    Asn1Node(
        mem::ByteBlock encodedData,
        unit::ByteLength headerLength,
        uint32_t tagNumber,
        util::List<Asn1Node> children) noexcept :
        _encodedData{std::move(encodedData)},
        _headerLength{headerLength},
        _tagNumber{tagNumber},
        _children{std::move(children)} {}

private:
    /// Get the absolute byte index in the shared byte storage.
    [[nodiscard]] auto encodedStorageByteIndex() const noexcept -> unit::ByteIndex;

private:
    mem::ByteBlock _encodedData;    ///< Complete canonical DER encoding.
    unit::ByteLength _headerLength; ///< Length of identifier and length octets.
    uint32_t _tagNumber{};          ///< Decoded numeric tag.
    util::List<Asn1Node> _children; ///< Direct child nodes.
};

}
