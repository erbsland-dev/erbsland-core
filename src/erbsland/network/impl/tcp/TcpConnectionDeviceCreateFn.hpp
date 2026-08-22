// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionDevice_fwd.hpp"
#include "TcpConnectionDeviceCallbacks_fwd.hpp"

#include "../../../event/EventLoopDriver_fwd.hpp"
#include "../../../unit/ByteLength.hpp"

#include <functional>

namespace erbsland::network::impl {

/// A factory for an injectable native TCP connection device.
using TcpConnectionDeviceCreateFn =
    std::function<TcpConnectionDevicePtr(event::EventLoopDriverPtr, unit::ByteLength, TcpConnectionDeviceCallbacks)>;

}
