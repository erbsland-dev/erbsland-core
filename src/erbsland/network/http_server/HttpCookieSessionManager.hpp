// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpCookieSessionManager_fwd.hpp"
#include "HttpCookieSessionManagerOptions.hpp"
#include "HttpServerSessionManager.hpp"

#include "../impl/http/cookie/HttpCookieSessionManagerData_fwd.hpp"

#include <memory>

namespace erbsland::network {

/// A bounded server-side session registry selected through one opaque cookie.
/// @tested{HttpCookieSessionManagerTest}
class HttpCookieSessionManager final : public HttpServerSessionManager {
public:
    /// Create a manager with validated cookie, lifetime, and capacity options.
    /// @throws err::ParameterError If an option is invalid.
    [[nodiscard]] static auto create(HttpCookieSessionManagerOptions options = {}) -> HttpCookieSessionManagerPtr;

public:
    // defaults
    ~HttpCookieSessionManager() override = default;

public: // implement HttpServerSessionManager
    /// Select an existing registered identifier or create a fresh session and cookie.
    [[nodiscard]] auto selectSession(const HttpServerSessionContext &context) -> HttpServerSessionSelection override;
    /// Remove an invalidated session and create its deletion-cookie fields.
    [[nodiscard]] auto sessionInvalidated(const HttpServerSessionPtr &session) -> HttpHeaders override;
    [[nodiscard]] auto renewSession(const HttpServerSessionPtr &session) -> HttpServerSessionRenewal override;

private:
    /// Create a manager after validation in its implementation data.
    explicit HttpCookieSessionManager(HttpCookieSessionManagerOptions options);

private:
    std::shared_ptr<impl::HttpCookieSessionManagerData> _data; ///< Internal bounded registry.
};

}
