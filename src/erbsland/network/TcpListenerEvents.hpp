// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Network_fwd.hpp"
#include "NetworkCallbacks.hpp"

#include "../event/EventEditor.hpp"

namespace erbsland::network {

/// Callback editor for a TCP listener.
/// @notest{Abstract interface; mock and native listener implementations own behavior tests.}
class TcpListenerEvents : public event::EventEditor {
public:
    // defaults
    ~TcpListenerEvents() override = default;

public:
    /// Set the listening-ready callback.
    /// @param callback The callback invoked after binding and listening succeeds.
    /// @return This editor for chaining.
    virtual auto onListening(NetworkEventFn callback) -> TcpListenerEvents & = 0;
    /// Set the incoming-connection callback.
    /// @param callback The callback receiving pending requests.
    /// @return This editor for chaining.
    virtual auto onConnection(TcpConnectionRequestFn callback) -> TcpListenerEvents & = 0;
    /// Set the closure callback.
    /// @param callback The callback invoked after the listener closes.
    /// @return This editor for chaining.
    virtual auto onClosed(NetworkEventFn callback) -> TcpListenerEvents & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> TcpListenerEvents & = 0;

protected:
    /// Create an editor for an event collection.
    /// @param targetEvents The target event collection.
    explicit TcpListenerEvents(event::EventsPtr targetEvents) : EventEditor{std::move(targetEvents)} {}
};

}
