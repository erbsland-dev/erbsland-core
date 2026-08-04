// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionCloseContext_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving a normal TCP connection closure.
using TcpConnectionCloseFn = std::function<void(const TcpConnectionCloseContext &)>;

}
