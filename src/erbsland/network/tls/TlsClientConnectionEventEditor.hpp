// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnectionCloseFn.hpp"
#include "TlsClientConnectionEventEditor_fwd.hpp"

#include "../source/NetworkDataFn.hpp"
#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"
#include "../tcp/TcpHostResolvedFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for an authenticated TLS client connection.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsClientConnectionEventEditor : public event::impl::CommonEventEditor {
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
    /// Set the authenticated application-data callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onData(NetworkDataFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the callback for renewed application send capacity.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the callback for completed bidirectional TLS closure.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onClosed(TlsClientConnectionCloseFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the callback for one operational failure.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> TlsClientConnectionEventEditor & = 0;
    /// Set the exactly-once terminal callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onFinal(NetworkEventFn callback) -> TlsClientConnectionEventEditor & = 0;

protected:
    /// Create a TLS client event editor.
    /// @param source The source whose handlers are edited.
    /// @param target The event target on which callbacks run.
    TlsClientConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
