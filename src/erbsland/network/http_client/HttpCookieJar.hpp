// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpCookie.hpp"
#include "HttpCookieInsertionOptions.hpp"
#include "HttpCookieJar_fwd.hpp"
#include "HttpCookieJarOptions.hpp"

#include "../url/Url.hpp"

#include "../../text/String.hpp"

#include <vector>

namespace erbsland::network {

/// A stable session-owned HTTP cookie jar.
/// @seedoc{/reference/network/http_client}
/// @tested{HttpCookieJarTest HttpClientTest}
class HttpCookieJar {
public: // defaults
    virtual ~HttpCookieJar() = default;

public:
    /// Get the current finite storage limits.
    [[nodiscard]] virtual auto options() const noexcept -> HttpCookieJarOptions = 0;
    /// Replace finite limits and immediately evict excess least-recently-used entries.
    virtual void setOptions(HttpCookieJarOptions options) = 0;
    /// Return immutable snapshots in deterministic creation order.
    [[nodiscard]] virtual auto cookies() -> std::vector<HttpCookie> = 0;
    /// Insert or replace one cookie as if it was received for the given URL.
    /// @throws err::ParameterError If the cookie or its attributes are invalid.
    virtual void setCookie(
        const Url &url, text::String name, text::String value, HttpCookieInsertionOptions options = {}) = 0;
    /// Remove the exact name/domain/path tuple and report whether it existed.
    virtual auto removeCookie(const text::String &name, const text::String &domain, const text::String &path)
        -> bool = 0;
    /// Remove all cookies.
    virtual void clear() noexcept = 0;
};

}
