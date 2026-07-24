// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Network_fwd.hpp"
#include "NetworkCallbacks.hpp"

#include "../event/EventEditor.hpp"

namespace erbsland::network {

/// Callback editor for a connected TCP byte stream.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnectionEvents : public event::EventEditor {
public:
    // defaults
    ~TcpConnectionEvents() override = default;

public:
    /// Set the received-data callback.
    /// @param callback The callback receiving owned stream chunks.
    /// @return This editor for chaining.
    virtual auto onData(NetworkDataFn callback) -> TcpConnectionEvents & = 0;
    /// Set the writable-transition callback.
    /// @param callback The callback invoked when capacity returns after back pressure.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> TcpConnectionEvents & = 0;
    /// Set the graceful-closure callback.
    /// @param callback The callback invoked after the stream closes.
    /// @return This editor for chaining.
    virtual auto onClosed(NetworkEventFn callback) -> TcpConnectionEvents & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> TcpConnectionEvents & = 0;

protected:
    /// Create an editor for an event collection.
    /// @param targetEvents The target event collection.
    explicit TcpConnectionEvents(event::EventsPtr targetEvents) : EventEditor{std::move(targetEvents)} {}
};

}
