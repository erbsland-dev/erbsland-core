// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerAcceptOptions.hpp"
#include "TlsServerConnection_fwd.hpp"
#include "TlsServerConnectionEventEditor.hpp"
#include "TlsServerConnectionState.hpp"

#include "../HostName.hpp"
#include "../IpEndpoint.hpp"
#include "../source/NetworkSendStatus.hpp"
#include "../source/SocketBufferLimits.hpp"
#include "../tcp/TcpConnectionRequest_fwd.hpp"

#include "../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../cryptology/tls/TlsSignatureScheme.hpp"
#include "../../event/EventSource.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::network {

/// A one-shot authenticated TLS 1.3 server connection over an accepted TCP request.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsServerConnection : public event::EventSource {
public: // defaults
    ~TlsServerConnection() override = default;

public: // metadata
    /// Get the selected requested application TLS configuration label.
    [[nodiscard]] virtual auto requestedConfigurationLabel() const -> std::optional<text::String> = 0;
    /// Get the selected exact registry label after fallback.
    [[nodiscard]] virtual auto matchedConfigurationLabel() const -> std::optional<text::String> = 0;
    /// Get the resolved local TCP endpoint.
    [[nodiscard]] virtual auto localEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the resolved remote TCP endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the canonical SNI name, if offered.
    [[nodiscard]] virtual auto serverName() const -> std::optional<HostName> = 0;
    /// Get the bounded client ALPN offer in wire order.
    [[nodiscard]] virtual auto offeredAlpn() const -> std::vector<mem::ByteBlock> = 0;
    /// Get the selected ALPN identifier, or an empty block.
    [[nodiscard]] virtual auto negotiatedAlpn() const -> mem::ByteBlock = 0;
    /// Get the negotiated cipher suite.
    [[nodiscard]] virtual auto cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> = 0;
    /// Get the selected CertificateVerify signature scheme.
    [[nodiscard]] virtual auto signatureScheme() const -> std::optional<cryptology::TlsSignatureScheme> = 0;
    /// Get the captured TLS protocol queue limits.
    [[nodiscard]] virtual auto bufferLimits() const noexcept -> SocketBufferLimits = 0;
    /// Get the current lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> TlsServerConnectionState = 0;

public: // operations
    /// Resolve immutable identity configurations and consume one pending TCP request.
    /// @param request The transferable pending TCP request.
    /// @param options The complete accepted TLS policy and required handshake quota.
    /// @throws err::ParameterError If the options, mappings, quota, or request are invalid.
    /// @throws err::RuntimeError If a required immutable TLS identity cannot be resolved.
    virtual void accept(TcpConnectionRequestPtr request, TlsServerAcceptOptions options) = 0;
    /// Atomically submit one complete authenticated application block.
    [[nodiscard]] virtual auto send(const mem::ByteBlock &data) -> NetworkSendStatus = 0;
    /// Suspend authenticated application-data delivery.
    virtual void pauseReceiving() = 0;
    /// Resume authenticated application-data delivery.
    virtual void resumeReceiving() = 0;
    /// Start bidirectional close_notify shutdown.
    virtual void close() = 0;
    /// Abort immediately; only the final callback is emitted.
    virtual void abort() noexcept = 0;
    /// Access the stable source-owned event editor.
    [[nodiscard]] auto events() -> TlsServerConnectionEventEditor & override = 0;

protected:
    /// Create an accepted TLS source owned by one event target.
    explicit TlsServerConnection(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
