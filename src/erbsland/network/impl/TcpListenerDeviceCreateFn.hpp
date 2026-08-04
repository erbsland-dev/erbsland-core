// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpListenerDevice_fwd.hpp"
#include "TcpListenerDeviceCallbacks_fwd.hpp"

#include "../../event/EventLoopDriver_fwd.hpp"

#include <functional>

namespace erbsland::network::impl {

/// A factory for an injectable native TCP listener device.
using TcpListenerDeviceCreateFn =
    std::function<TcpListenerDevicePtr(event::EventLoopDriverPtr, TcpListenerDeviceCallbacks)>;

}
