// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class HttpServerSessionManager;

/// A shared HTTP server session manager.
using HttpServerSessionManagerPtr = std::shared_ptr<HttpServerSessionManager>;

}
