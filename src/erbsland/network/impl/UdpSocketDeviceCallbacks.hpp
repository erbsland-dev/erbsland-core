// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocketDeviceCallbacks_fwd.hpp"

#include "../source/NetworkErrorContext.hpp"
#include "../udp/UdpDatagram.hpp"
#include "../udp/UdpDatagramDropContext.hpp"

#include <functional>

namespace erbsland::network::impl {

/// Native UDP device callbacks delivered on the owner event-loop thread.
/// @notest{Callback bundle exercised through concrete UDP device suites.}
struct UdpSocketDeviceCallbacks final {
    std::function<void(UdpDatagram)> datagram;                   ///< One received datagram.
    std::function<void(UdpDatagramDropContext)> datagramDropped; ///< One locally discarded datagram.
    std::function<void()> sendCompleted;                         ///< Completion of one pending send.
    std::function<void()> writable;                              ///< Native output became writable.
    std::function<void(NetworkErrorContext)> error;              ///< Native operational failure.
};

}
