// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionRequest_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving a pending incoming TCP connection request.
using TcpConnectionRequestFn = std::function<void(TcpConnectionRequestPtr)>;

}
