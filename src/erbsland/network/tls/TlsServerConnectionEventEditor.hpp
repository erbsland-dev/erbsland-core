// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerConnectionEventEditor_fwd.hpp"

#include "../source/ConnectionEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for an accepted authenticated TLS server connection.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsServerConnectionEventEditor : public ConnectionEventEditor {
public: // defaults
    ~TlsServerConnectionEventEditor() override = default;

public:
    /// Set the checkpoint after the accepted TCP transport becomes active.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onTransportConnected(NetworkEventFn callback) -> TlsServerConnectionEventEditor & = 0;
    /// Set the checkpoint after bounded ClientHello parsing and policy selection.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onClientHello(NetworkEventFn callback) -> TlsServerConnectionEventEditor & = 0;
    /// Set the checkpoint after authenticated client Finished.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onHandshakeCompleted(NetworkEventFn callback) -> TlsServerConnectionEventEditor & = 0;

public: // implement ConnectionEventEditor
    auto onData(NetworkDataFn callback) -> TlsServerConnectionEventEditor & override = 0;
    auto onWritable(NetworkEventFn callback) -> TlsServerConnectionEventEditor & override = 0;
    auto onClosed(ConnectionCloseFn callback) -> TlsServerConnectionEventEditor & override = 0;
    auto onError(NetworkErrorFn callback) -> TlsServerConnectionEventEditor & override = 0;
    auto onFinal(NetworkEventFn callback) -> TlsServerConnectionEventEditor & override = 0;

protected:
    /// Create a TLS server event editor.
    /// @param source The source whose handlers are edited.
    /// @param target The event target on which callbacks run.
    TlsServerConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        ConnectionEventEditor{std::move(source), std::move(target)} {}
};

}
