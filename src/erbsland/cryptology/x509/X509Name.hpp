// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509RelativeDistinguishedName.hpp"

#include "../asn1/Asn1Node.hpp"
#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../impl/X509Parser_fwd.hpp"

#include "../../text/String.hpp"
#include "../../text/StringList.hpp"
#include "../../util/List.hpp"

namespace erbsland::cryptology {

/// An ordered X.509 issuer or subject Name.
/// @tested{X509CertificateTest}
class X509Name final {
    friend class impl::X509Parser;

public:
    /// Create an empty name.
    X509Name() = default;

public: // tests
    /// Test if the name contains no relative distinguished names.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _rdns.isEmpty(); }

public: // accessors
    /// Get ordered relative distinguished names.
    [[nodiscard]] auto relativeDistinguishedNames() const noexcept
        -> const util::List<X509RelativeDistinguishedName> & {
        return _rdns;
    }
    /// Get all decoded values for an attribute OID.
    /// @param oid The attribute type to select.
    /// @return Values in certificate order.
    [[nodiscard]] auto values(const Asn1ObjectIdentifier &oid) const -> text::StringList;
    /// Get common-name values.
    [[nodiscard]] auto commonNames() const -> text::StringList;
    /// Get organization values.
    [[nodiscard]] auto organizations() const -> text::StringList;
    /// Get organizational-unit values.
    [[nodiscard]] auto organizationalUnits() const -> text::StringList;
    /// Get locality values.
    [[nodiscard]] auto localities() const -> text::StringList;
    /// Get state or province values.
    [[nodiscard]] auto states() const -> text::StringList;
    /// Get country values.
    [[nodiscard]] auto countries() const -> text::StringList;
    /// Get the exact ASN.1 Name node.
    [[nodiscard]] auto asn1() const noexcept -> const Asn1Node & { return _node; }

public: // conversion
    /// Return a deterministic RFC 4514-style display string.
    [[nodiscard]] auto toString() const -> text::String;

private:
    /// Create a parsed X.509 distinguished name.
    X509Name(util::List<X509RelativeDistinguishedName> rdns, Asn1Node node) noexcept :
        _rdns{std::move(rdns)}, _node{std::move(node)} {}
    /// Get the display name for an X.509 attribute identifier.
    [[nodiscard]] static auto attributeName(const Asn1ObjectIdentifier &oid) -> text::String;
    /// Escape an attribute value for X.509 name rendering.
    [[nodiscard]] static auto escapedValue(const text::String &value) -> text::String;

private:
    util::List<X509RelativeDistinguishedName> _rdns; ///< Ordered relative distinguished names.
    Asn1Node _node;                                  ///< Exact ASN.1 Name node.
};

}
