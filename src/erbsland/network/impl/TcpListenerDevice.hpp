// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpListenerDevice_fwd.hpp"
#include "TcpListenerDeviceCallbacks.hpp"

#include "../IpEndpoint.hpp"

#include "../../event/EventLoopDriver_fwd.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::network::impl {

/// Platform-neutral interface for one native TCP listening socket.
/// @tested{TcpListenerTest TcpSocketLiveTest}
class TcpListenerDevice {
public:
    // defaults
    virtual ~TcpListenerDevice() = default;

public: // factory
    /// Create the default native TCP listener device.
    [[nodiscard]] static auto create(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks)
        -> TcpListenerDevicePtr;

public:
    /// Bind and listen on a local endpoint.
    /// @return The actual bound endpoint.
    [[nodiscard]] virtual auto start(IpEndpoint localEndpoint, unit::ItemCount backlog) -> IpEndpoint = 0;
    /// Enable or disable native accept operations.
    virtual void setAccepting(bool enabled) = 0;
    /// Close the native listening socket.
    virtual void close() noexcept = 0;
    /// Abort the native listening socket immediately.
    virtual void abort() noexcept = 0;
};

}
