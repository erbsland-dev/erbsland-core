// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsConfiguration_fwd.hpp"
#include "TlsServerIdentity.hpp"

#include "../x509/X509ServerCertificatePolicy.hpp"

#include <optional>
#include <utility>

namespace erbsland::cryptology {

/// Application-owned material and policies used to establish TLS connections.
/// Registered configurations are copied into immutable snapshots. A server identity is retained through immutable
/// shared ownership so copies remain cheap and cannot copy its move-only private key.
/// @tested{CryptologyConfigurationTest TlsServerIdentityTest}
class TlsConfiguration final {
public:
    /// Create an empty TLS configuration.
    TlsConfiguration() = default;
    /// Create a TLS configuration with a server-certificate authentication policy.
    /// @param serverCertificatePolicy The policy copied into this configuration.
    explicit TlsConfiguration(X509ServerCertificatePolicy serverCertificatePolicy) noexcept :
        _serverCertificatePolicy{std::move(serverCertificatePolicy)} {}

    // defaults
    ~TlsConfiguration() = default;
    TlsConfiguration(const TlsConfiguration &) = default;
    TlsConfiguration(TlsConfiguration &&) noexcept = default;
    auto operator=(const TlsConfiguration &) -> TlsConfiguration & = default;
    auto operator=(TlsConfiguration &&) noexcept -> TlsConfiguration & = default;

public:
    /// Test whether this configuration can authenticate TLS server certificates.
    [[nodiscard]] auto hasServerCertificatePolicy() const noexcept -> bool {
        return _serverCertificatePolicy.has_value();
    }
    /// Get the server-certificate authentication policy.
    /// @return The policy, or no value if it was not configured.
    [[nodiscard]] auto serverCertificatePolicy() const noexcept -> const std::optional<X509ServerCertificatePolicy> & {
        return _serverCertificatePolicy;
    }
    /// Set the server-certificate authentication policy.
    /// @param policy The replacement policy.
    void setServerCertificatePolicy(X509ServerCertificatePolicy policy) noexcept {
        _serverCertificatePolicy = std::move(policy);
    }
    /// Clear the server-certificate authentication policy.
    void clearServerCertificatePolicy() noexcept { _serverCertificatePolicy.reset(); }

public: // server identity
    /// Test whether this configuration contains a TLS server identity.
    [[nodiscard]] auto hasServerIdentity() const noexcept -> bool { return _serverIdentity != nullptr; }
    /// Get the immutable shared TLS server identity, or a null pointer when absent.
    [[nodiscard]] auto serverIdentity() const noexcept -> const TlsServerIdentityConstPtr & { return _serverIdentity; }
    /// Replace the TLS server identity and transfer ownership of its move-only key.
    /// @param identity A completely validated server identity.
    void setServerIdentity(TlsServerIdentity identity) {
        _serverIdentity = std::make_shared<const TlsServerIdentity>(std::move(identity));
    }
    /// Clear the TLS server identity from this configuration snapshot source.
    void clearServerIdentity() noexcept { _serverIdentity.reset(); }

private:
    std::optional<X509ServerCertificatePolicy> _serverCertificatePolicy; ///< TLS server authentication policy.
    TlsServerIdentityConstPtr _serverIdentity;                           ///< Optional immutable shared server identity.
};

}
