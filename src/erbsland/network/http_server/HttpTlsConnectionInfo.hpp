// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HostName.hpp"

#include "../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../cryptology/tls/TlsSignatureScheme.hpp"
#include "../../text/String.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::network {

/// An immutable snapshot of negotiated TLS information for an HTTP connection.
/// @tested{HttpServerLiveTest}
class HttpTlsConnectionInfo final {
public:
    /// Create a TLS information snapshot.
    HttpTlsConnectionInfo(
        std::optional<text::String> requestedConfigurationLabel,
        std::optional<text::String> matchedConfigurationLabel,
        std::optional<HostName> serverName,
        std::vector<text::String> offeredAlpn,
        text::String negotiatedAlpn,
        std::optional<cryptology::TlsCipherSuite> cipherSuite,
        std::optional<cryptology::TlsSignatureScheme> signatureScheme) :
        _requestedConfigurationLabel{std::move(requestedConfigurationLabel)},
        _matchedConfigurationLabel{std::move(matchedConfigurationLabel)},
        _serverName{std::move(serverName)},
        _offeredAlpn{std::move(offeredAlpn)},
        _negotiatedAlpn{std::move(negotiatedAlpn)},
        _cipherSuite{std::move(cipherSuite)},
        _signatureScheme{std::move(signatureScheme)} {}

public:
    /// Get the requested application TLS configuration label.
    [[nodiscard]] auto requestedConfigurationLabel() const noexcept -> const std::optional<text::String> & {
        return _requestedConfigurationLabel;
    }
    /// Get the matched exact TLS configuration label.
    [[nodiscard]] auto matchedConfigurationLabel() const noexcept -> const std::optional<text::String> & {
        return _matchedConfigurationLabel;
    }
    /// Get the canonical SNI server name, if offered.
    [[nodiscard]] auto serverName() const noexcept -> const std::optional<HostName> & { return _serverName; }
    /// Get the bounded ALPN offer in wire order.
    [[nodiscard]] auto offeredAlpn() const noexcept -> const std::vector<text::String> & { return _offeredAlpn; }
    /// Get the negotiated ALPN identifier, or an empty string.
    [[nodiscard]] auto negotiatedAlpn() const noexcept -> const text::String & { return _negotiatedAlpn; }
    /// Get the negotiated TLS cipher suite.
    [[nodiscard]] auto cipherSuite() const noexcept -> const std::optional<cryptology::TlsCipherSuite> & {
        return _cipherSuite;
    }
    /// Get the selected CertificateVerify signature scheme.
    [[nodiscard]] auto signatureScheme() const noexcept -> const std::optional<cryptology::TlsSignatureScheme> & {
        return _signatureScheme;
    }

private:
    std::optional<text::String> _requestedConfigurationLabel;       ///< Requested application label.
    std::optional<text::String> _matchedConfigurationLabel;         ///< Matched registry label.
    std::optional<HostName> _serverName;                            ///< Canonical SNI server name.
    std::vector<text::String> _offeredAlpn;                         ///< Client ALPN offer.
    text::String _negotiatedAlpn;                                   ///< Negotiated ALPN identifier.
    std::optional<cryptology::TlsCipherSuite> _cipherSuite;         ///< Negotiated cipher suite.
    std::optional<cryptology::TlsSignatureScheme> _signatureScheme; ///< Certificate signature scheme.
};

}
