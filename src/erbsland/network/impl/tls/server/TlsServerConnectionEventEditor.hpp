// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerConnection_fwd.hpp"

#include "../../../tls/TlsServerConnectionEventEditor.hpp"

namespace erbsland::network::impl {

/// Event editor for the built-in accepted TLS connection.
/// @tested{TlsServerConnectionTest}
class TlsServerConnectionEventEditor final : public network::TlsServerConnectionEventEditor {
public:
    /// Create an editor for one built-in accepted TLS connection.
    /// @param source The source that owns the callbacks.
    /// @param target The event target on which callbacks run.
    TlsServerConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target);

public:
    auto onTransportConnected(NetworkEventFn callback) -> network::TlsServerConnectionEventEditor & override;
    auto onClientHello(NetworkEventFn callback) -> network::TlsServerConnectionEventEditor & override;
    auto onHandshakeCompleted(NetworkEventFn callback) -> network::TlsServerConnectionEventEditor & override;
    auto onData(NetworkDataFn callback) -> network::TlsServerConnectionEventEditor & override;
    auto onWritable(NetworkEventFn callback) -> network::TlsServerConnectionEventEditor & override;
    auto onClosed(ConnectionCloseFn callback) -> network::TlsServerConnectionEventEditor & override;
    auto onError(NetworkErrorFn callback) -> network::TlsServerConnectionEventEditor & override;
    auto onFinal(NetworkEventFn callback) -> network::TlsServerConnectionEventEditor & override;

private:
    /// Get the concrete source associated with this editor.
    /// @return The built-in TLS server connection.
    [[nodiscard]] auto connection() const -> std::shared_ptr<TlsServerConnection>;
};

}
