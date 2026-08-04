// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerConnectionCloseFn.hpp"
#include "TlsServerConnectionEventEditor_fwd.hpp"

#include "../source/NetworkDataFn.hpp"
#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for an accepted authenticated TLS server connection.
/// @notest{Abstract interface; the built-in implementation owns behavior tests.}
class TlsServerConnectionEventEditor : public event::impl::CommonEventEditor {
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
    /// Set the authenticated application-data callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onData(NetworkDataFn callback) -> TlsServerConnectionEventEditor & = 0;
    /// Set the callback for renewed application send capacity.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> TlsServerConnectionEventEditor & = 0;
    /// Set the callback for completed bidirectional TLS closure.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onClosed(TlsServerConnectionCloseFn callback) -> TlsServerConnectionEventEditor & = 0;
    /// Set the callback for one operational failure.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> TlsServerConnectionEventEditor & = 0;
    /// Set the exactly-once terminal callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onFinal(NetworkEventFn callback) -> TlsServerConnectionEventEditor & = 0;

protected:
    /// Create a TLS server event editor.
    /// @param source The source whose handlers are edited.
    /// @param target The event target on which callbacks run.
    TlsServerConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
