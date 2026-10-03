// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../cryptology/keys/KeyAgreementAlgorithm.hpp"
#include "../../../../cryptology/tls/TlsIdentity_fwd.hpp"
#include "../../../../cryptology/x509/X509TlsCertificatePolicy.hpp"
#include "../../../../text/String.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../../time/Timestamp.hpp"
#include "../../../Host.hpp"
#include "../../../source/SocketBufferLimits.hpp"

#include <utility>
#include <vector>

namespace erbsland::network::impl {

/// Immutable configuration for the internal TLS 1.3 client protocol core.
/// @tested{TlsClientProtocolTest}
class TlsClientProtocolOptions final {
public:
    /// Create protocol options with explicit trust and validation time.
    /// @param host Original DNS or IP reference identity used for SNI and certificate validation.
    /// @param certificatePolicy Explicit-anchor server certificate policy.
    /// @param validationTime Exact certificate-validation time.
    /// @param alpnProtocols Optional ALPN identifiers in preference order.
    /// @param bufferLimits Aggregate encrypted output and authenticated/buffered input limits.
    /// @param clientIdentity Optional explicitly selected client identity; never searched automatically.
    /// @param supportedGroups Nonempty, duplicate-free ordered key-agreement groups.
    TlsClientProtocolOptions(
        Host host,
        cryptology::X509TlsCertificatePolicy certificatePolicy,
        time::Timestamp validationTime,
        text::StringList alpnProtocols = {},
        SocketBufferLimits bufferLimits = {},
        std::vector<cryptology::KeyAgreementAlgorithm> supportedGroups =
            {cryptology::KeyAgreementAlgorithm::X25519, cryptology::KeyAgreementAlgorithm::P256},
        cryptology::TlsIdentityConstPtr clientIdentity = {}) noexcept :
        _clientIdentity{std::move(clientIdentity)},
        _host{std::move(host)},
        _certificatePolicy{std::move(certificatePolicy)},
        _validationTime{validationTime},
        _alpnProtocols{std::move(alpnProtocols)},
        _supportedGroups{std::move(supportedGroups)},
        _bufferLimits{bufferLimits} {}

    // defaults
    ~TlsClientProtocolOptions() = default;
    TlsClientProtocolOptions(const TlsClientProtocolOptions &) = default;
    TlsClientProtocolOptions(TlsClientProtocolOptions &&) noexcept = default;
    auto operator=(const TlsClientProtocolOptions &) -> TlsClientProtocolOptions & = default;
    auto operator=(TlsClientProtocolOptions &&) noexcept -> TlsClientProtocolOptions & = default;

public: // accessors
    /// Get the original DNS or IP reference identity.
    [[nodiscard]] auto host() const noexcept -> const Host & { return _host; }
    /// Get the explicit server-certificate policy.
    [[nodiscard]] auto certificatePolicy() const noexcept -> const cryptology::X509TlsCertificatePolicy & {
        return _certificatePolicy;
    }
    /// Get the certificate-validation time.
    [[nodiscard]] auto validationTime() const noexcept -> time::Timestamp { return _validationTime; }
    /// Get offered ALPN identifiers.
    [[nodiscard]] auto alpnProtocols() const noexcept -> const text::StringList & { return _alpnProtocols; }
    /// Get aggregate socket buffer limits.
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }

public: // identity
    /// Get the explicitly configured client identity, or no identity.
    [[nodiscard]] auto clientIdentity() const noexcept -> const cryptology::TlsIdentityConstPtr & {
        return _clientIdentity;
    }

public: // groups
    /// Get the supported group preference list; the first group supplies the initial key share.
    [[nodiscard]] auto supportedGroups() const noexcept -> const std::vector<cryptology::KeyAgreementAlgorithm> & {
        return _supportedGroups;
    }

private:
    cryptology::TlsIdentityConstPtr _clientIdentity;         ///< Explicit identity permitted for client authentication.
    Host _host;                                              ///< Original reference identity.
    cryptology::X509TlsCertificatePolicy _certificatePolicy; ///< Explicit trust and chain policy.
    time::Timestamp _validationTime;                         ///< Exact validation instant.
    text::StringList _alpnProtocols;                         ///< Optional ALPN offer.
    std::vector<cryptology::KeyAgreementAlgorithm> _supportedGroups; ///< Ordered supported groups.
    SocketBufferLimits _bufferLimits;                                ///< Aggregate queue limits.
};

}
