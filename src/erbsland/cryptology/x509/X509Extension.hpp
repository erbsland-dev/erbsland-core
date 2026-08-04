// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../asn1/Asn1Node.hpp"
#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../impl/X509Parser_fwd.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::cryptology {

/// One X.509 extension preserving its exact value and decoded inner node.
/// @tested{X509CertificateTest}
class X509Extension final {
    friend class impl::X509Parser;

public:
    /// Create an empty extension.
    X509Extension() = default;

public: // accessors
    /// Get the extension OID.
    [[nodiscard]] auto oid() const noexcept -> const Asn1ObjectIdentifier & { return _oid; }
    /// Test if the extension is marked critical.
    [[nodiscard]] auto isCritical() const noexcept -> bool { return _critical; }
    /// Get the raw extnValue octets.
    [[nodiscard]] auto value() const noexcept -> const mem::ByteBlock & { return _value; }
    /// Get the decoded inner ASN.1 root, or an empty node if decoding failed in compatible mode.
    [[nodiscard]] auto asn1() const noexcept -> const Asn1Node & { return _innerNode; }

private:
    /// Create a parsed X.509 extension.
    X509Extension(Asn1ObjectIdentifier oid, bool critical, mem::ByteBlock value, Asn1Node innerNode) noexcept :
        _oid{std::move(oid)}, _critical{critical}, _value{std::move(value)}, _innerNode{std::move(innerNode)} {}

private:
    Asn1ObjectIdentifier _oid; ///< Extension OID.
    bool _critical{};          ///< Critical flag.
    mem::ByteBlock _value;     ///< Raw extnValue octets.
    Asn1Node _innerNode;       ///< Decoded inner value.
};

}
