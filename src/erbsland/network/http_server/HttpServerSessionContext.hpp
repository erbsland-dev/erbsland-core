// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerRequest_fwd.hpp"
#include "HttpServerSession_fwd.hpp"

#include "../../text/String.hpp"

#include <functional>
#include <optional>
#include <utility>

namespace erbsland::network {

/// Session-resolution context for one request.
/// @tested{HttpCookieSessionManagerTest}
class HttpServerSessionContext final {
public:
    /// Server-owned session factory used by synchronous managers.
    using CreateFn = std::function<HttpServerSessionPtr(std::optional<text::String>, HttpSessionDataPtr)>;

public:
    /// Create one request-specific manager context.
    HttpServerSessionContext(HttpServerRequestPtr request, bool secure, CreateFn createFn) :
        _request{std::move(request)}, _secure{secure}, _createFn{std::move(createFn)} {}

public:
    /// Get the complete retained request and its connection context.
    [[nodiscard]] auto request() const noexcept -> const HttpServerRequestPtr & { return _request; }
    /// Test whether the request uses an authenticated TLS connection.
    [[nodiscard]] auto isSecure() const noexcept -> bool { return _secure; }
    /// Create a server-owned logical session with optional identity and data.
    [[nodiscard]] auto createSession(std::optional<text::String> identifier = {}, HttpSessionDataPtr data = {}) const
        -> HttpServerSessionPtr {
        return _createFn(std::move(identifier), std::move(data));
    }

private:
    HttpServerRequestPtr _request; ///< Request being resolved.
    bool _secure{};                ///< Whether its connection is authenticated TLS.
    CreateFn _createFn;            ///< Server-owned session factory.
};

}
