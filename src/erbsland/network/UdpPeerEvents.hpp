// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Network_fwd.hpp"
#include "NetworkCallbacks.hpp"

#include "../event/EventEditor.hpp"

namespace erbsland::network {

/// Callback editor for a connected UDP peer.
/// @notest{Abstract interface; mock and native UDP implementations own behavior tests.}
class UdpPeerEvents : public event::EventEditor {
public:
    // defaults
    ~UdpPeerEvents() override = default;

public:
    /// Set the peer-ready callback.
    /// @param callback The callback invoked after resolution and setup succeeds.
    /// @return This editor for chaining.
    virtual auto onReady(NetworkEventFn callback) -> UdpPeerEvents & = 0;
    /// Set the received-data callback.
    /// @param callback The callback receiving owned payloads.
    /// @return This editor for chaining.
    virtual auto onData(NetworkDataFn callback) -> UdpPeerEvents & = 0;
    /// Set the writable-transition callback.
    /// @param callback The callback invoked when capacity returns after back pressure.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> UdpPeerEvents & = 0;
    /// Set the closure callback.
    /// @param callback The callback invoked after the peer closes.
    /// @return This editor for chaining.
    virtual auto onClosed(NetworkEventFn callback) -> UdpPeerEvents & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> UdpPeerEvents & = 0;

protected:
    /// Create an editor for an event collection.
    /// @param targetEvents The target event collection.
    explicit UdpPeerEvents(event::EventsPtr targetEvents) : EventEditor{std::move(targetEvents)} {}
};

}
