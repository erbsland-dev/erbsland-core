// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerProtocolConfiguration.hpp"

#include "../../../../cryptology/keys/KeyAgreementAlgorithm.hpp"
#include "../../../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../../../cryptology/tls/TlsIdentity_fwd.hpp"
#include "../../../../text/String.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../HostName.hpp"
#include "../../../source/SocketBufferLimits.hpp"

#include <utility>
#include <vector>

namespace erbsland::network::impl {

/// Immutable configuration for the internal TLS 1.3 server protocol core.
/// Cipher suites and ALPN identifiers are ordered by server preference. The RFC 8446 server profile owns one default
/// complete server configuration plus bounded exact-SNI configurations. PSK, early data and post-handshake
/// authentication are unsupported.
/// Specification: RFC 8446 Sections 4.1.1 and 4.4.2; RFC 7301 Section 3.2.
/// @tested{TlsServerProtocolTest}
class TlsServerProtocolOptions final {
public:
    /// One exact canonical SNI configuration mapping.
    struct NamedConfiguration final {
        HostName serverName;                          ///< Exact canonical SNI name.
        TlsServerProtocolConfiguration configuration; ///< Complete selected server profile.
    };

public:
    /// Create validated server protocol options.
    /// @param configuration The default server configuration. Its identity must not be null; its chain must contain at
    /// most 16 certificates and fit a 1 MiB Certificate message.
    /// @param alpnProtocols At most 64 unique ALPN identifiers in server-preference order, each 1-255 bytes and
    /// together at most 4096 encoded bytes.
    /// @param cipherSuites A non-empty list of supported, unique cipher suites in server-preference order.
    /// @param bufferLimits Aggregate encrypted output and authenticated/buffered input limits.
    /// @param supportedGroups Nonempty, duplicate-free ordered key-agreement groups.
    TlsServerProtocolOptions(
        TlsServerProtocolConfiguration configuration,
        text::StringList alpnProtocols = {},
        std::vector<cryptology::TlsCipherSuite> cipherSuites = defaultCipherSuites(),
        SocketBufferLimits bufferLimits = {},
        std::vector<cryptology::KeyAgreementAlgorithm> supportedGroups = {
            cryptology::KeyAgreementAlgorithm::X25519, cryptology::KeyAgreementAlgorithm::P256});
    /// Create validated server protocol options with exact SNI mappings.
    /// @param defaultConfiguration The profile for absent or unmatched SNI. Its identity must not be null and must
    /// satisfy the 16-certificate and 1 MiB Certificate-message limits.
    /// @param namedConfigurations At most 64 unique exact canonical SNI profiles. Each identity must not be null and
    /// must satisfy the certificate-flight limits.
    /// @param alpnProtocols At most 64 unique ALPN identifiers in server-preference order, each 1-255 bytes and
    /// together at most 4096 encoded bytes.
    /// @param cipherSuites A non-empty list of supported, unique cipher suites in server-preference order.
    /// @param bufferLimits Aggregate encrypted output and authenticated/buffered input limits.
    /// @param supportedGroups Nonempty, duplicate-free ordered key-agreement groups.
    [[nodiscard]] static auto withNamedConfigurations(
        TlsServerProtocolConfiguration defaultConfiguration,
        std::vector<NamedConfiguration> namedConfigurations,
        text::StringList alpnProtocols = {},
        std::vector<cryptology::TlsCipherSuite> cipherSuites = defaultCipherSuites(),
        SocketBufferLimits bufferLimits = {},
        std::vector<cryptology::KeyAgreementAlgorithm> supportedGroups = {
            cryptology::KeyAgreementAlgorithm::X25519,
            cryptology::KeyAgreementAlgorithm::P256}) -> TlsServerProtocolOptions;

    // defaults
    ~TlsServerProtocolOptions() = default;
    TlsServerProtocolOptions(const TlsServerProtocolOptions &) = default;
    TlsServerProtocolOptions(TlsServerProtocolOptions &&) noexcept = default;
    auto operator=(const TlsServerProtocolOptions &) -> TlsServerProtocolOptions & = default;
    auto operator=(TlsServerProtocolOptions &&) noexcept -> TlsServerProtocolOptions & = default;

public: // accessors
    /// Get the default server configuration.
    [[nodiscard]] auto defaultConfiguration() const noexcept -> const TlsServerProtocolConfiguration & {
        return _defaultConfiguration;
    }
    /// Get exact canonical SNI configuration mappings.
    [[nodiscard]] auto namedConfigurations() const noexcept -> const std::vector<NamedConfiguration> & {
        return _namedConfigurations;
    }
    /// Get supported ALPN identifiers in server-preference order.
    [[nodiscard]] auto alpnProtocols() const noexcept -> const text::StringList & { return _alpnProtocols; }
    /// Get enabled cipher suites in server-preference order.
    [[nodiscard]] auto cipherSuites() const noexcept -> const std::vector<cryptology::TlsCipherSuite> & {
        return _cipherSuites;
    }
    /// Get supported key-agreement groups in server-preference order.
    [[nodiscard]] auto supportedGroups() const noexcept -> const std::vector<cryptology::KeyAgreementAlgorithm> & {
        return _supportedGroups;
    }
    /// Get aggregate socket buffer limits.
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }

private:
    /// Build the library-default cipher-suite preference list.
    [[nodiscard]] static auto defaultCipherSuites() -> std::vector<cryptology::TlsCipherSuite>;
    /// Validate one configured server identity and its certificate flight bounds.
    static void validateConfiguration(const TlsServerProtocolConfiguration &configuration);

private:
    TlsServerProtocolConfiguration _defaultConfiguration;            ///< Default immutable certificate and signing key.
    std::vector<NamedConfiguration> _namedConfigurations;            ///< Exact canonical SNI profiles.
    text::StringList _alpnProtocols;                                 ///< Server-preference ALPN identifiers.
    std::vector<cryptology::TlsCipherSuite> _cipherSuites;           ///< Server-preference enabled suites.
    std::vector<cryptology::KeyAgreementAlgorithm> _supportedGroups; ///< Group preference order.
    SocketBufferLimits _bufferLimits;                                ///< Aggregate queue bounds.
};

}
