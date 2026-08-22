// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class HttpServerSession;
class HttpSessionData;

/// A shared HTTP server session.
using HttpServerSessionPtr = std::shared_ptr<HttpServerSession>;
/// A weak HTTP server session.
using HttpServerSessionWeakPtr = std::weak_ptr<HttpServerSession>;
/// Shared application-defined session data.
using HttpSessionDataPtr = std::shared_ptr<HttpSessionData>;

}
