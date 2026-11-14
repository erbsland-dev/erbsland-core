// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpListenerDevice_fwd.hpp"
#include "TcpListenerDeviceCallbacks.hpp"

#include "../../../event/EventLoopDriver_fwd.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../IpEndpoint.hpp"

namespace erbsland::network::impl {

/// Platform-neutral interface for one native TCP listening socket.
/// Callers retain shared ownership throughout each operation and explicitly close or abort before releasing the device.
/// @tested{TcpListenerTest TcpSocketLiveTest}
class TcpListenerDevice {
public:
    // defaults
    virtual ~TcpListenerDevice() = default;

public: // factory
    /// Create the default native TCP listener device.
    /// @param driver The platform's default native event-loop driver; custom drivers are not supported.
    /// @param callbacks The source callbacks.
    /// @return The platform-native listener device.
    [[nodiscard]] static auto create(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks)
        -> TcpListenerDevicePtr;

public:
    /// Bind and listen on a local endpoint.
    /// @param localEndpoint The local endpoint to bind.
    /// @param backlog The maximum pending-connection queue size.
    /// @return The actual bound endpoint.
    /// @throws NetworkError If socket creation, configuration, binding, or listening fails.
    /// @throws system::PlatformError If native event registration fails.
    [[nodiscard]] virtual auto start(IpEndpoint localEndpoint, unit::ItemCount backlog) -> IpEndpoint = 0;
    /// Enable or disable native accept operations.
    /// @param enabled Whether to accept connections.
    /// @throws system::PlatformError If native event registration fails.
    virtual void setAccepting(bool enabled) = 0;
    /// Close the native listening socket.
    virtual void close() noexcept = 0;
    /// Abort the native listening socket immediately.
    virtual void abort() noexcept = 0;
};

}
