// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionFilterResult_fwd.hpp"

namespace erbsland::network {

/// The synchronous admission decision for an incoming TCP connection.
enum class TcpConnectionFilterResult : std::uint8_t {
    Accept, ///< Create and emit a pending connection request.
    Reject, ///< Close the incoming connection immediately.
};

}
