// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnection_fwd.hpp"
#include "TlsClientConnectionEventEditor.hpp"
#include "TlsClientConnectOptions.hpp"

#include "../HostEndpoint.hpp"
#include "../source/Connection.hpp"

#include "../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../cryptology/x509/X509Certificate.hpp"
#include "../../text/String.hpp"
#include "../../util/List.hpp"

#include <optional>

namespace erbsland::network {

/// A one-shot authenticated TLS 1.3 client connection over TCP.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsClientConnection : public Connection {
public: // defaults
    ~TlsClientConnection() override = default;

public: // metadata
    /// Get the label requested when the connection started.
    [[nodiscard]] virtual auto requestedConfigurationLabel() const -> text::String = 0;
    /// Get the exact registry label selected by fallback.
    [[nodiscard]] virtual auto matchedConfigurationLabel() const -> text::String = 0;
    /// Get the originally requested endpoint.
    [[nodiscard]] virtual auto requestedEndpoint() const -> std::optional<HostEndpoint> = 0;
    /// Get the negotiated cipher suite.
    [[nodiscard]] virtual auto cipherSuite() const -> std::optional<cryptology::TlsCipherSuite> = 0;
    /// Get the selected ALPN identifier, or an empty string when none was selected.
    [[nodiscard]] virtual auto negotiatedAlpn() const -> text::String = 0;
    /// Get the authenticated target-to-anchor peer certificate path.
    [[nodiscard]] virtual auto peerCertificatePath() const -> util::List<cryptology::X509Certificate> = 0;

public: // operations
    /// Resolve configuration and connect using default options.
    void connect(HostEndpoint endpoint);
    /// Resolve configuration synchronously and start the one-shot connection.
    /// @throws err::ParameterError If options or the endpoint are invalid.
    /// @throws err::RuntimeError If no complete client TLS configuration resolves.
    virtual void connect(HostEndpoint endpoint, TlsClientConnectOptions options) = 0;

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
    [[nodiscard]] auto events() -> TlsClientConnectionEventEditor & override = 0;

protected:
    /// Create a TLS client source owned by one event target.
    /// @param ownerEvents The owner event target.
    explicit TlsClientConnection(event::EventsPtr ownerEvents) : Connection{std::move(ownerEvents)} {}
};

}
