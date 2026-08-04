// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionRequestState_fwd.hpp"

namespace erbsland::network {

/// The decision state of a pending incoming TCP connection.
enum class TcpConnectionRequestState : std::uint8_t {
    Pending,  ///< No accept-or-reject decision was made yet.
    Accepted, ///< A prepared connection claimed the request.
    Rejected, ///< The request was rejected or abandoned.
};

}
