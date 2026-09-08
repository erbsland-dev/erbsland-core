// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpConnectionInfo_fwd.hpp"

#include "../source/NetworkErrorContext_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving an immutable HTTP connection snapshot.
/// @tested{HttpServerLiveTest}
using HttpConnectionInfoFn = std::function<void(const HttpConnectionInfo &)>;

/// A callback receiving a connection-local error and its immutable HTTP connection snapshot.
/// @tested{HttpServerLiveTest}
using HttpConnectionErrorFn = std::function<void(const HttpConnectionInfo &, const NetworkErrorContext &)>;

}
