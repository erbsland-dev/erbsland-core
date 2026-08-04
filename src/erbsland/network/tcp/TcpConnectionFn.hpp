// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnection_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving a newly connected TCP stream.
using TcpConnectionFn = std::function<void(TcpConnectionPtr)>;

}
