// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionEventEditor_fwd.hpp"
#include "TcpHostResolvedFn.hpp"

#include "../source/ConnectionEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for a connected TCP byte stream.
/// The connection owns this stable editor; its common base exposes the source and callback target to generic code.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnectionEventEditor : public ConnectionEventEditor {
public:
    /// Set the host-resolution callback.
    /// This callback runs before any native connection begins and may call `abort()` on the connection.
    /// @param callback The callback receiving ordered resolved endpoints.
    /// @return This editor for chaining.
    virtual auto onHostResolved(TcpHostResolvedFn callback) -> TcpConnectionEventEditor & = 0;
    /// Set the connected callback.
    /// @param callback The callback invoked after an outgoing or accepted stream becomes active.
    /// @return This editor for chaining.
    virtual auto onConnected(NetworkEventFn callback) -> TcpConnectionEventEditor & = 0;
    // defaults
    ~TcpConnectionEventEditor() override = default;

public: // implement ConnectionEventEditor
    auto onData(NetworkDataFn callback) -> TcpConnectionEventEditor & override = 0;
    auto onWritable(NetworkEventFn callback) -> TcpConnectionEventEditor & override = 0;
    auto onClosed(ConnectionCloseFn callback) -> TcpConnectionEventEditor & override = 0;
    auto onError(NetworkErrorFn callback) -> TcpConnectionEventEditor & override = 0;
    auto onFinal(NetworkEventFn callback) -> TcpConnectionEventEditor & override = 0;

protected:
    /// Create an editor for a TCP connection.
    /// @param source The connection that owns the editor.
    /// @param target The callback target.
    TcpConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        ConnectionEventEditor{std::move(source), std::move(target)} {}
};

}
