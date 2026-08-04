// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpDatagramDropFn.hpp"
#include "UdpDatagramFn.hpp"
#include "UdpSocketEventEditor_fwd.hpp"

#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for a bound UDP socket.
/// The socket owns this stable editor; its common base exposes the source and callback target to generic code.
/// @tested{UdpSocketTest UdpSocketLiveTest}
class UdpSocketEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~UdpSocketEventEditor() override = default;

public:
    /// Set the bound-ready callback.
    /// @param callback The callback invoked after binding succeeds.
    /// @return This editor for chaining.
    virtual auto onBound(NetworkEventFn callback) -> UdpSocketEventEditor & = 0;
    /// Set the received-datagram callback.
    /// @param callback The callback receiving owned datagrams.
    /// @return This editor for chaining.
    virtual auto onDatagram(UdpDatagramFn callback) -> UdpSocketEventEditor & = 0;
    /// Set the discarded-datagram callback.
    /// @param callback The callback receiving details about locally discarded datagrams.
    /// @return This editor for chaining.
    virtual auto onDatagramDropped(UdpDatagramDropFn callback) -> UdpSocketEventEditor & = 0;
    /// Set the writable-transition callback.
    /// @param callback The callback invoked when capacity can satisfy a previously blocked send.
    /// @return This editor for chaining.
    virtual auto onWritable(NetworkEventFn callback) -> UdpSocketEventEditor & = 0;
    /// Set the closure callback.
    /// @param callback The callback invoked after the socket closes.
    /// @return This editor for chaining.
    virtual auto onClosed(NetworkEventFn callback) -> UdpSocketEventEditor & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> UdpSocketEventEditor & = 0;

protected:
    /// Create an editor for a UDP socket.
    /// @param source The socket that owns the editor.
    /// @param target The callback target.
    UdpSocketEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
