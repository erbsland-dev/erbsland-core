// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocket_fwd.hpp"

#include "../../udp/UdpSocketEventEditor.hpp"

namespace erbsland::network::impl {

/// Event editor for the native UDP socket implementation.
/// @tested{UdpSocketTest UdpSocketLiveTest}
class UdpSocketEventEditor final : public network::UdpSocketEventEditor {
public:
    /// Create the source-owned editor.
    /// @param source The UDP socket that owns this editor.
    /// @param target The owner-loop callback target.
    UdpSocketEventEditor(event::EventSourcePtr source, event::EventsPtr target);

public: // implement network::UdpSocketEventEditor
    auto onBound(NetworkEventFn callback) -> network::UdpSocketEventEditor & override;
    auto onDatagram(UdpDatagramFn callback) -> network::UdpSocketEventEditor & override;
    auto onDatagramDropped(UdpDatagramDropFn callback) -> network::UdpSocketEventEditor & override;
    auto onWritable(NetworkEventFn callback) -> network::UdpSocketEventEditor & override;
    auto onClosed(NetworkEventFn callback) -> network::UdpSocketEventEditor & override;
    auto onError(NetworkErrorFn callback) -> network::UdpSocketEventEditor & override;

private:
    /// Lock and return the UDP socket that owns this editor.
    [[nodiscard]] auto socket() const -> std::shared_ptr<UdpSocket>;
};

}
