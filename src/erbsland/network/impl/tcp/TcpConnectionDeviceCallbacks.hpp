// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionDeviceCallbacks_fwd.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../IpEndpoint.hpp"
#include "../../source/NetworkErrorContext.hpp"

#include <functional>

namespace erbsland::network::impl {

/// Callbacks emitted by a native TCP connection device.
/// @tested{TcpConnectionTest TcpSocketLiveTest}
struct TcpConnectionDeviceCallbacks final {
    std::function<void(IpEndpoint, IpEndpoint)> connected; ///< Native connection became active.
    std::function<void(mem::ByteBlock)> data;              ///< Stream data was received.
    std::function<void()> sendCompleted;                   ///< Retained output completed.
    std::function<void()> remoteClosed;                    ///< The remote stream ended normally.
    std::function<void(NetworkErrorContext)> error;        ///< A native operation failed.
};

}
