// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../cryptology/x509/X509ServerCertificatePolicy.hpp"
#include "../../../../text/String.hpp"
#include "../../../../time/DateTime.hpp"
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
    TlsClientProtocolOptions(
        Host host,
        cryptology::X509ServerCertificatePolicy certificatePolicy,
        time::DateTime validationTime,
        std::vector<text::String> alpnProtocols = {},
        SocketBufferLimits bufferLimits = {}) noexcept :
        _host{std::move(host)},
        _certificatePolicy{std::move(certificatePolicy)},
        _validationTime{validationTime},
        _alpnProtocols{std::move(alpnProtocols)},
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
    [[nodiscard]] auto certificatePolicy() const noexcept -> const cryptology::X509ServerCertificatePolicy & {
        return _certificatePolicy;
    }
    /// Get the certificate-validation time.
    [[nodiscard]] auto validationTime() const noexcept -> time::DateTime { return _validationTime; }
    /// Get offered ALPN identifiers.
    [[nodiscard]] auto alpnProtocols() const noexcept -> const std::vector<text::String> & { return _alpnProtocols; }
    /// Get aggregate socket buffer limits.
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }

private:
    Host _host;                                                 ///< Original reference identity.
    cryptology::X509ServerCertificatePolicy _certificatePolicy; ///< Explicit trust and chain policy.
    time::DateTime _validationTime;                             ///< Exact validation instant.
    std::vector<text::String> _alpnProtocols;                   ///< Optional ALPN offer.
    SocketBufferLimits _bufferLimits;                           ///< Aggregate queue limits.
};

}
