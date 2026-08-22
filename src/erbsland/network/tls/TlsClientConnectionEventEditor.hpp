// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnectionEventEditor_fwd.hpp"

#include "../source/ConnectionEventEditor.hpp"
#include "../tcp/TcpHostResolvedFn.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for an authenticated TLS client connection.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsClientConnectionEventEditor : public ConnectionEventEditor {
public: // defaults
    ~TlsClientConnectionEventEditor() override = default;

public:
    /// Set the checkpoint invoked after DNS resolution produced transport candidates.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onHostResolved(TcpHostResolvedFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the checkpoint invoked after TCP establishment and before TLS startup.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onTransportConnected(NetworkEventFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the checkpoint invoked after authenticated parsing of EncryptedExtensions.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onPeerHello(NetworkEventFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the checkpoint invoked after certificate validation and CertificateVerify.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onPeerAuthenticated(NetworkEventFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the checkpoint invoked after verified server Finished and queued client Finished.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onHandshakeCompleted(NetworkEventFn callback) -> TlsClientConnectionEventEditor & = 0;

public: // implement ConnectionEventEditor
    auto onData(NetworkDataFn callback) -> TlsClientConnectionEventEditor & override = 0;
    auto onWritable(NetworkEventFn callback) -> TlsClientConnectionEventEditor & override = 0;
    auto onClosed(ConnectionCloseFn callback) -> TlsClientConnectionEventEditor & override = 0;
    auto onError(NetworkErrorFn callback) -> TlsClientConnectionEventEditor & override = 0;
    auto onFinal(NetworkEventFn callback) -> TlsClientConnectionEventEditor & override = 0;

protected:
    /// Create a TLS client event editor.
    /// @param source The source whose handlers are edited.
    /// @param target The event target on which callbacks run.
    TlsClientConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        ConnectionEventEditor{std::move(source), std::move(target)} {}
};

}
