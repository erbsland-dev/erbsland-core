// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509Name.hpp"

#include "../asn1/Asn1Node.hpp"
#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../impl/X509Parser_fwd.hpp"

#include "../../network/IpAddress.hpp"
#include "../../text/String.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::cryptology {

/// A supported or preserved GeneralName from an X.509 extension.
/// @tested{X509CertificateTest}
class X509GeneralName final {
    friend class impl::X509Parser;

public:
    /// The represented GeneralName alternative.
    enum class Kind : uint8_t {
        Unsupported,  ///< A preserved unsupported context-specific alternative.
        Email,        ///< An rfc822Name value.
        Dns,          ///< A dNSName value.
        Directory,    ///< A directoryName value.
        Uri,          ///< A uniformResourceIdentifier value.
        IpAddress,    ///< An iPAddress value.
        RegisteredId, ///< A registeredID value.
    };

public:
    /// Create an empty unsupported name.
    X509GeneralName() = default;

public: // accessors
    /// Get the represented alternative.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Get an email, DNS, or URI value.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get an IP address value.
    [[nodiscard]] auto ipAddress() const noexcept -> std::optional<network::IpAddress> { return _ipAddress; }
    /// Get a directory-name value.
    [[nodiscard]] auto directoryName() const noexcept -> const X509Name & { return _directoryName; }
    /// Get a registered-ID value.
    [[nodiscard]] auto registeredId() const noexcept -> const Asn1ObjectIdentifier & { return _registeredId; }
    /// Get the exact context-specific ASN.1 node.
    [[nodiscard]] auto asn1() const noexcept -> const Asn1Node & { return _node; }

private:
    /// Create a parsed general-name alternative.
    X509GeneralName(
        Kind kind,
        text::String text,
        std::optional<network::IpAddress> ipAddress,
        X509Name directoryName,
        Asn1ObjectIdentifier registeredId,
        Asn1Node node) noexcept :
        _kind{kind},
        _text{std::move(text)},
        _ipAddress{std::move(ipAddress)},
        _directoryName{std::move(directoryName)},
        _registeredId{std::move(registeredId)},
        _node{std::move(node)} {}

private:
    Kind _kind{Kind::Unsupported};                ///< The represented alternative.
    text::String _text;                           ///< Text value.
    std::optional<network::IpAddress> _ipAddress; ///< IP address value.
    X509Name _directoryName;                      ///< Directory name value.
    Asn1ObjectIdentifier _registeredId;           ///< Registered ID value.
    Asn1Node _node;                               ///< Exact ASN.1 node.
};

}
