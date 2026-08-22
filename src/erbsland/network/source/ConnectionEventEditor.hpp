// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConnectionCloseFn.hpp"
#include "ConnectionEventEditor_fwd.hpp"
#include "NetworkDataFn.hpp"
#include "NetworkErrorFn.hpp"
#include "NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for the active data phase of a byte-stream connection.
/// @tested{ConnectionTest TcpConnectionTest TlsClientConnectionTest TlsServerConnectionTest}
class ConnectionEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~ConnectionEventEditor() override = default;

public:
    /// Set the received application-data callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onData(NetworkDataFn callback) -> ConnectionEventEditor & = 0;
    /// Set the writable-transition callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> ConnectionEventEditor & = 0;
    /// Set the orderly-closure callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onClosed(ConnectionCloseFn callback) -> ConnectionEventEditor & = 0;
    /// Set the operational-error callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> ConnectionEventEditor & = 0;
    /// Set the exactly-once terminal callback.
    /// @param callback The replacement callback.
    /// @return This editor for chaining.
    virtual auto onFinal(NetworkEventFn callback) -> ConnectionEventEditor & = 0;

protected:
    /// Create an editor for one connection.
    /// @param source The connection that owns the editor.
    /// @param target The callback target.
    ConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
