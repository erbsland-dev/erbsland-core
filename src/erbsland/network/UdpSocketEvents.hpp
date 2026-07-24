// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Network_fwd.hpp"
#include "NetworkCallbacks.hpp"

#include "../event/EventEditor.hpp"

namespace erbsland::network {

/// Callback editor for a bound UDP socket.
/// @notest{Abstract interface; mock and native UDP implementations own behavior tests.}
class UdpSocketEvents : public event::EventEditor {
public:
    // defaults
    ~UdpSocketEvents() override = default;

public:
    /// Set the bound-ready callback.
    /// @param callback The callback invoked after binding succeeds.
    /// @return This editor for chaining.
    virtual auto onBound(NetworkEventFn callback) -> UdpSocketEvents & = 0;
    /// Set the received-datagram callback.
    /// @param callback The callback receiving owned datagrams.
    /// @return This editor for chaining.
    virtual auto onDatagram(UdpDatagramFn callback) -> UdpSocketEvents & = 0;
    /// Set the writable-transition callback.
    /// @param callback The callback invoked when capacity returns after back pressure.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> UdpSocketEvents & = 0;
    /// Set the closure callback.
    /// @param callback The callback invoked after the socket closes.
    /// @return This editor for chaining.
    virtual auto onClosed(NetworkEventFn callback) -> UdpSocketEvents & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> UdpSocketEvents & = 0;

protected:
    /// Create an editor for an event collection.
    /// @param targetEvents The target event collection.
    explicit UdpSocketEvents(event::EventsPtr targetEvents) : EventEditor{std::move(targetEvents)} {}
};

}
