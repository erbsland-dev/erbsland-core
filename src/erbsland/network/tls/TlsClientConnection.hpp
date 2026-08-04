// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnection_fwd.hpp"
#include "TlsClientConnectionEventEditor.hpp"
#include "TlsClientConnectionState.hpp"
#include "TlsClientConnectOptions.hpp"

#include "../HostEndpoint.hpp"
#include "../IpEndpoint.hpp"
#include "../source/NetworkSendStatus.hpp"
#include "../source/SocketBufferLimits.hpp"

#include "../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../cryptology/x509/X509Certificate.hpp"
#include "../../event/EventSource.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../util/List.hpp"

#include <optional>

namespace erbsland::network {

/// A one-shot authenticated TLS 1.3 client connection over TCP.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsClientConnection : public event::EventSource {
public: // defaults
    ~TlsClientConnection() override = default;

public: // metadata
    /// Get the label requested when the connection started.
    [[nodiscard]] virtual auto requestedConfigurationLabel() const -> text::String = 0;
    /// Get the exact registry label selected by fallback.
    [[nodiscard]] virtual auto matchedConfigurationLabel() const -> text::String = 0;
    /// Get the originally requested endpoint.
    [[nodiscard]] virtual auto requestedEndpoint() const -> std::optional<HostEndpoint> = 0;
    /// Get the resolved local TCP endpoint.
    [[nodiscard]] virtual auto localEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the resolved remote TCP endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the negotiated cipher suite.
    [[nodiscard]] virtual auto cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> = 0;
    /// Get the selected ALPN identifier, or an empty block when none was selected.
    [[nodiscard]] virtual auto negotiatedAlpn() const -> mem::ByteBlock = 0;
    /// Get the authenticated target-to-anchor peer certificate path.
    [[nodiscard]] virtual auto peerCertificatePath() const -> util::List<cryptology::X509Certificate> = 0;
    /// Get the captured TLS protocol queue limits.
    [[nodiscard]] virtual auto bufferLimits() const noexcept -> SocketBufferLimits = 0;
    /// Get the current TLS connection lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> TlsClientConnectionState = 0;

public: // operations
    /// Resolve configuration and connect using default options.
    void connect(HostEndpoint endpoint);
    /// Resolve configuration synchronously and start the one-shot connection.
    /// @throws err::ParameterError If options or the endpoint are invalid.
    /// @throws err::RuntimeError If no complete client TLS configuration resolves.
    virtual void connect(HostEndpoint endpoint, TlsClientConnectOptions options) = 0;
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
    [[nodiscard]] auto events() -> TlsClientConnectionEventEditor & override = 0;

protected:
    /// Create a TLS client source owned by one event target.
    /// @param ownerEvents The owner event target.
    explicit TlsClientConnection(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
