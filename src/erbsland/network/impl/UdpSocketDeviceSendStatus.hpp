// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocketDeviceSendStatus_fwd.hpp"

namespace erbsland::network::impl {

/// The immediate result of submitting a datagram to a native UDP device.
enum class UdpSocketDeviceSendStatus : std::uint8_t {
    Complete,   ///< The native device completed the datagram immediately.
    Pending,    ///< The native device will report completion asynchronously.
    WouldBlock, ///< The native device requires a writable notification before retrying.
};

}
