// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnection_fwd.hpp"

#include "../../../tls/TlsClientConnectionEventEditor.hpp"

namespace erbsland::network::impl {

/// Event editor for the built-in TLS client connection.
/// @tested{TlsClientConnectionTest}
class TlsClientConnectionEventEditor final : public network::TlsClientConnectionEventEditor {
public:
    /// Create an editor for one built-in TLS client connection.
    /// @param source The source that owns the callbacks.
    /// @param target The event target on which callbacks run.
    TlsClientConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target);

public:
    auto onHostResolved(TcpHostResolvedFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onTransportConnected(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onPeerHello(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onPeerAuthenticated(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onHandshakeCompleted(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onData(NetworkDataFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onWritable(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onClosed(ConnectionCloseFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onError(NetworkErrorFn callback) -> network::TlsClientConnectionEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> network::TlsClientConnectionEventEditor & override;

private:
    /// Get the concrete source associated with this editor.
    /// @return The built-in TLS client connection.
    [[nodiscard]] auto connection() const -> std::shared_ptr<TlsClientConnection>;
};

}
