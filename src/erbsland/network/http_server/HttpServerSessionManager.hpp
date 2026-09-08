// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerSessionContext.hpp"
#include "HttpServerSessionManager_fwd.hpp"
#include "HttpServerSessionRenewal.hpp"
#include "HttpServerSessionSelection.hpp"

namespace erbsland::network {

/// Synchronously selects a logical session for each HTTP server request.
/// Implementations run on the server owner loop and must not block.
/// @tested{HttpCookieSessionManagerTest HttpServerLiveTest}
class HttpServerSessionManager {
public:
    // defaults
    virtual ~HttpServerSessionManager() = default;

public:
    /// Select or create one logical session and bounded eventual-response fields.
    [[nodiscard]] virtual auto selectSession(const HttpServerSessionContext &context) -> HttpServerSessionSelection = 0;
    /// Remove manager state and return fields for the current invalidation response.
    [[nodiscard]] virtual auto sessionInvalidated(const HttpServerSessionPtr &session) -> HttpHeaders = 0;
    /// Replace the identifier of an existing valid session.
    [[nodiscard]] virtual auto renewSession(const HttpServerSessionPtr &session) -> HttpServerSessionRenewal = 0;
};

}
