// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../impl/X509Parser_fwd.hpp"

#include "../../data/asn1/Asn1Node.hpp"
#include "../../data/asn1/Asn1ObjectIdentifier.hpp"
#include "../../text/String.hpp"

namespace erbsland::cryptology {

/// One attribute in an X.509 relative distinguished name.
/// @tested{X509CertificateTest}
class X509NameAttribute final {
public:
    /// Create an empty name attribute.
    X509NameAttribute() = default;

public: // accessors
    /// Get the attribute type OID.
    [[nodiscard]] auto oid() const noexcept -> const data::asn1::Asn1ObjectIdentifier & { return _oid; }
    /// Get the decoded display value, or an empty string when unavailable.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }
    /// Get the exact ASN.1 value node.
    [[nodiscard]] auto asn1() const noexcept -> const data::asn1::Asn1Node & { return _node; }

private:
    friend class impl::X509Parser;
    /// Create a parsed distinguished-name attribute.
    X509NameAttribute(data::asn1::Asn1ObjectIdentifier oid, text::String value, data::asn1::Asn1Node node) noexcept :
        _oid{std::move(oid)}, _value{std::move(value)}, _node{std::move(node)} {}

private:
    data::asn1::Asn1ObjectIdentifier _oid; ///< Attribute type OID.
    text::String _value;                   ///< Decoded display value.
    data::asn1::Asn1Node _node;            ///< Exact ASN.1 value.
};

}
