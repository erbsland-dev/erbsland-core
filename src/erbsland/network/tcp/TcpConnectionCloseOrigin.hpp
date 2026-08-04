// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionCloseOrigin_fwd.hpp"

namespace erbsland::network {

/// The side that initiated a normal TCP connection closure.
enum class TcpConnectionCloseOrigin : std::uint8_t {
    Local,  ///< The local application requested closure first.
    Remote, ///< The remote peer ended its byte stream first.
};

}
