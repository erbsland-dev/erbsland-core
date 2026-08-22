// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocketDevice_fwd.hpp"
#include "UdpSocketDeviceCallbacks.hpp"

#include "../../../event/EventLoopDriver_fwd.hpp"

#include <functional>

namespace erbsland::network::impl {

/// A factory callback for injecting a native UDP socket device.
using UdpSocketDeviceCreateFn = std::function<UdpSocketDevicePtr(event::EventLoopDriverPtr, UdpSocketDeviceCallbacks)>;

}
