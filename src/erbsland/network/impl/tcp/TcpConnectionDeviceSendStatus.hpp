// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionDeviceSendStatus_fwd.hpp"

namespace erbsland::network::impl {

/// The result of submitting one complete stream block to a native TCP device.
enum class TcpConnectionDeviceSendStatus : std::uint8_t {
    Complete, ///< The complete block was written synchronously.
    Pending,  ///< The device retained the block until a completion callback.
};

}
