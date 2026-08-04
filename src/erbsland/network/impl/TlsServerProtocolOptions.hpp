// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HostName.hpp"
#include "../source/SocketBufferLimits.hpp"

#include "../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../cryptology/tls/TlsServerIdentity_fwd.hpp"
#include "../../mem/ByteBlock.hpp"

#include <utility>
#include <vector>

namespace erbsland::network::impl {

/// Immutable configuration for the internal TLS 1.3 server protocol core.
/// Cipher suites and ALPN identifiers are ordered by server preference. The RFC 8446 server profile owns one default
/// immutable identity plus bounded exact-SNI identities and deliberately has no PSK, early-data, HelloRetryRequest, or
/// client-authentication policy.
/// Specification: RFC 8446 Sections 4.1.1 and 4.4.2; RFC 7301 Section 3.2.
/// @tested{TlsServerProtocolTest}
class TlsServerProtocolOptions final {
public:
    /// One exact canonical SNI identity mapping.
    struct NamedIdentity final {
        HostName serverName;                            ///< Exact canonical SNI name.
        cryptology::TlsServerIdentityConstPtr identity; ///< Immutable selected identity.
    };

public:
    /// Create validated server protocol options.
    /// @param identity The default immutable server certificate chain and protected signing key.
    /// @param alpnProtocols Supported opaque ALPN identifiers in server-preference order.
    /// @param cipherSuites Enabled cipher suites in server-preference order.
    /// @param bufferLimits Aggregate encrypted output and authenticated/buffered input limits.
    /// @throws err::ParameterError If identity, suite, ALPN, or certificate-flight bounds are invalid.
    TlsServerProtocolOptions(
        cryptology::TlsServerIdentityConstPtr identity,
        std::vector<mem::ByteBlock> alpnProtocols = {},
        std::vector<cryptology::TlsCipherSuite> cipherSuites = defaultCipherSuites(),
        SocketBufferLimits bufferLimits = {});
    /// Create validated server protocol options with exact SNI mappings.
    /// @param defaultIdentity The identity selected for absent or unmatched SNI.
    /// @param namedIdentities At most 64 unique exact canonical SNI identities.
    /// @param alpnProtocols Supported opaque ALPN identifiers in server-preference order.
    /// @param cipherSuites Enabled cipher suites in server-preference order.
    /// @param bufferLimits Aggregate encrypted output and authenticated/buffered input limits.
    /// @throws err::ParameterError If an identity, name, suite, ALPN, or certificate-flight bound is invalid.
    [[nodiscard]] static auto withNamedIdentities(
        cryptology::TlsServerIdentityConstPtr defaultIdentity,
        std::vector<NamedIdentity> namedIdentities,
        std::vector<mem::ByteBlock> alpnProtocols = {},
        std::vector<cryptology::TlsCipherSuite> cipherSuites = defaultCipherSuites(),
        SocketBufferLimits bufferLimits = {}) -> TlsServerProtocolOptions;

    // defaults
    ~TlsServerProtocolOptions() = default;
    TlsServerProtocolOptions(const TlsServerProtocolOptions &) = default;
    TlsServerProtocolOptions(TlsServerProtocolOptions &&) noexcept = default;
    auto operator=(const TlsServerProtocolOptions &) -> TlsServerProtocolOptions & = default;
    auto operator=(TlsServerProtocolOptions &&) noexcept -> TlsServerProtocolOptions & = default;

public: // accessors
    /// Get the default immutable server identity.
    [[nodiscard]] auto defaultIdentity() const noexcept -> const cryptology::TlsServerIdentityConstPtr & {
        return _defaultIdentity;
    }
    /// Get exact canonical SNI identity mappings.
    [[nodiscard]] auto namedIdentities() const noexcept -> const std::vector<NamedIdentity> & {
        return _namedIdentities;
    }
    /// Get supported ALPN identifiers in server-preference order.
    [[nodiscard]] auto alpnProtocols() const noexcept -> const std::vector<mem::ByteBlock> & { return _alpnProtocols; }
    /// Get enabled cipher suites in server-preference order.
    [[nodiscard]] auto cipherSuites() const noexcept -> const std::vector<cryptology::TlsCipherSuite> & {
        return _cipherSuites;
    }
    /// Get aggregate socket buffer limits.
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }

private:
    /// Build the library-default cipher-suite preference list.
    [[nodiscard]] static auto defaultCipherSuites() -> std::vector<cryptology::TlsCipherSuite>;
    /// Validate one configured server identity and its certificate flight bounds.
    static void validateIdentity(const cryptology::TlsServerIdentityConstPtr &identity);

private:
    cryptology::TlsServerIdentityConstPtr _defaultIdentity; ///< Default immutable certificate and signing key.
    std::vector<NamedIdentity> _namedIdentities;            ///< Exact canonical SNI identities.
    std::vector<mem::ByteBlock> _alpnProtocols;             ///< Server-preference ALPN identifiers.
    std::vector<cryptology::TlsCipherSuite> _cipherSuites;  ///< Server-preference enabled suites.
    SocketBufferLimits _bufferLimits;                       ///< Aggregate queue bounds.
};

}
