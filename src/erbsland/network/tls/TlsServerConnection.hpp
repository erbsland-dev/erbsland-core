// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerAcceptOptions.hpp"
#include "TlsServerConnection_fwd.hpp"
#include "TlsServerConnectionEventEditor.hpp"

#include "../HostName.hpp"
#include "../source/Connection.hpp"
#include "../tcp/TcpConnectionRequest_fwd.hpp"

#include "../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../cryptology/tls/TlsSignatureScheme.hpp"
#include "../../cryptology/x509/X509Certificate.hpp"
#include "../../text/String.hpp"
#include "../../text/StringList.hpp"
#include "../../util/List.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::network {

/// A one-shot authenticated TLS 1.3 server connection over an accepted TCP request.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsServerConnection : public Connection {
public: // defaults
    ~TlsServerConnection() override = default;

public: // metadata
    /// Get the authenticated client path, empty until certificate authentication and Finished succeed.
    [[nodiscard]] virtual auto peerCertificatePath() const -> util::List<cryptology::X509Certificate> = 0;
    /// Test whether the completed handshake authenticated a client certificate.
    [[nodiscard]] virtual auto isPeerAuthenticated() const -> bool = 0;
    /// Get the selected requested application TLS configuration label.
    [[nodiscard]] virtual auto requestedConfigurationLabel() const -> std::optional<text::String> = 0;
    /// Get the selected exact registry label after fallback.
    [[nodiscard]] virtual auto matchedConfigurationLabel() const -> std::optional<text::String> = 0;
    /// Get the canonical SNI name, if offered.
    [[nodiscard]] virtual auto serverName() const -> std::optional<HostName> = 0;
    /// Get the bounded client ALPN offer in wire order.
    [[nodiscard]] virtual auto offeredAlpn() const -> text::StringList = 0;
    /// Get the selected ALPN identifier, or an empty block.
    [[nodiscard]] virtual auto negotiatedAlpn() const -> text::String = 0;
    /// Get the negotiated cipher suite.
    [[nodiscard]] virtual auto cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> = 0;
    /// Get the selected CertificateVerify signature scheme.
    [[nodiscard]] virtual auto signatureScheme() const -> std::optional<cryptology::TlsSignatureScheme> = 0;

public: // operations
    /// Resolve immutable identity configurations and consume one pending TCP request.
    /// @param request The transferable pending TCP request. Must not be null.
    /// @param options The complete accepted TLS policy. Must include a non-null handshake quota, valid identity
    /// mappings, and buffer limits and deadlines within `TlsServerAcceptOptions` bounds.
    /// @throws cryptology::CryptologyError If a default or named TLS configuration cannot be resolved or has no server
    /// identity.
    virtual void accept(TcpConnectionRequestPtr request, TlsServerAcceptOptions options) = 0;

public: // implement Connection
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override = 0;
    [[nodiscard]] auto remoteEndpoint() const -> std::optional<IpEndpoint> override = 0;
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits override = 0;
    [[nodiscard]] auto state() const noexcept -> ConnectionState override = 0;
    [[nodiscard]] auto send(const mem::ByteBlock &data) -> NetworkSendStatus override = 0;
    void pauseReceiving() override = 0;
    void resumeReceiving() override = 0;
    void close() override = 0;
    void abort() noexcept override = 0;
    [[nodiscard]] auto events() -> TlsServerConnectionEventEditor & override = 0;

protected:
    /// Create an accepted TLS source owned by one event target.
    explicit TlsServerConnection(event::EventTargetPtr ownerLoop) : Connection{std::move(ownerLoop)} {}
};

}
