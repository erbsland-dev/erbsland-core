// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionCloseFn.hpp"
#include "TcpConnectionEventEditor_fwd.hpp"
#include "TcpHostResolvedFn.hpp"

#include "../source/NetworkDataFn.hpp"
#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for a connected TCP byte stream.
/// The connection owns this stable editor; its common base exposes the source and callback target to generic code.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnectionEventEditor : public event::impl::CommonEventEditor {
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

public:
    /// Set the received-data callback.
    /// @param callback The callback receiving owned stream chunks.
    /// @return This editor for chaining.
    virtual auto onData(NetworkDataFn callback) -> TcpConnectionEventEditor & = 0;
    /// Set the writable-transition callback.
    /// @param callback The callback invoked when capacity returns after back pressure.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> TcpConnectionEventEditor & = 0;
    /// Set the graceful-closure callback.
    /// @param callback The callback invoked after the stream closes.
    /// @return This editor for chaining.
    virtual auto onClosed(TcpConnectionCloseFn callback) -> TcpConnectionEventEditor & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> TcpConnectionEventEditor & = 0;
    /// Set the final callback.
    /// @param callback The callback invoked after closure, failure, or explicit abort.
    /// @return This editor for chaining.
    virtual auto onFinal(NetworkEventFn callback) -> TcpConnectionEventEditor & = 0;

protected:
    /// Create an editor for a TCP connection.
    /// @param source The connection that owns the editor.
    /// @param target The callback target.
    TcpConnectionEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
