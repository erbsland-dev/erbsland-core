// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Network_fwd.hpp"
#include "NetworkCallbacks.hpp"

#include "../event/EventEditor.hpp"

namespace erbsland::network {

/// Callback editor for an outgoing TCP connection attempt.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnectionAttemptEvents : public event::EventEditor {
public:
    // defaults
    ~TcpConnectionAttemptEvents() override = default;

public:
    /// Set the successful-connection callback.
    /// @param callback The callback receiving the connected stream.
    /// @return This editor for chaining.
    virtual auto onConnected(TcpConnectionFn callback) -> TcpConnectionAttemptEvents & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> TcpConnectionAttemptEvents & = 0;

protected:
    /// Create an editor for an event collection.
    /// @param targetEvents The target event collection.
    explicit TcpConnectionAttemptEvents(event::EventsPtr targetEvents) : EventEditor{std::move(targetEvents)} {}
};

}
