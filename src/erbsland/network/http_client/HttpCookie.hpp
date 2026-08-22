// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../http/HttpCookieSameSite.hpp"

#include "../../text/String.hpp"
#include "../../time/DateTime.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// An immutable snapshot of one stored HTTP client cookie.
/// @tested{HttpCookieJarTest}
class HttpCookie final {
public:
    /// Create one cookie snapshot.
    HttpCookie(
        text::String name,
        text::String value,
        text::String domain,
        text::String path,
        std::optional<time::DateTime> expires,
        std::optional<HttpCookieSameSite> sameSite,
        bool hostOnly,
        bool secure,
        bool httpOnly) noexcept :
        _name{std::move(name)},
        _value{std::move(value)},
        _domain{std::move(domain)},
        _path{std::move(path)},
        _expires{std::move(expires)},
        _sameSite{sameSite},
        _hostOnly{hostOnly},
        _secure{secure},
        _httpOnly{httpOnly} {}

public:
    /// Get the cookie name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Get the cookie value.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }
    /// Get the canonical IDNA ASCII domain.
    [[nodiscard]] auto domain() const noexcept -> const text::String & { return _domain; }
    /// Get the cookie path.
    [[nodiscard]] auto path() const noexcept -> const text::String & { return _path; }
    /// Get the absolute expiry, if persistent.
    [[nodiscard]] auto expires() const noexcept -> const std::optional<time::DateTime> & { return _expires; }
    /// Get the retained SameSite attribute.
    [[nodiscard]] auto sameSite() const noexcept -> std::optional<HttpCookieSameSite> { return _sameSite; }
    /// Test whether this is a host-only cookie.
    [[nodiscard]] constexpr auto isHostOnly() const noexcept -> bool { return _hostOnly; }
    /// Test whether the Secure attribute is enabled.
    [[nodiscard]] constexpr auto isSecure() const noexcept -> bool { return _secure; }
    /// Test whether the HttpOnly attribute is enabled.
    [[nodiscard]] constexpr auto isHttpOnly() const noexcept -> bool { return _httpOnly; }

private:
    text::String _name;                          ///< Cookie name.
    text::String _value;                         ///< Cookie value.
    text::String _domain;                        ///< Canonical IDNA ASCII domain.
    text::String _path;                          ///< Request path prefix.
    std::optional<time::DateTime> _expires;      ///< Absolute expiry, if persistent.
    std::optional<HttpCookieSameSite> _sameSite; ///< Retained SameSite attribute.
    bool _hostOnly{};                            ///< Domain attribute was absent.
    bool _secure{};                              ///< Secure attribute.
    bool _httpOnly{};                            ///< HttpOnly attribute.
};

}
