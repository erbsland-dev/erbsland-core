// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptedSocket_fwd.hpp"
#include "TcpListenerDeviceCallbacks_fwd.hpp"

#include "../source/NetworkErrorContext.hpp"

#include <functional>

namespace erbsland::network::impl {

/// Callbacks emitted by a native TCP listener device.
/// @tested{TcpListenerTest TcpSocketLiveTest}
struct TcpListenerDeviceCallbacks final {
    std::function<void(TcpAcceptedSocketPtr)> accepted; ///< A native socket was accepted.
    std::function<void(NetworkErrorContext)> error;     ///< A native listener operation failed.
};

}
