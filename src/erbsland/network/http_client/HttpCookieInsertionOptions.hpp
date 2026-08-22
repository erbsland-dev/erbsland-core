// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../http/HttpCookieSameSite.hpp"

#include "../../text/String.hpp"
#include "../../time/DateTime.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// Attributes for synchronously inserting one cookie into a client jar.
/// @tested{HttpCookieJarTest}
class HttpCookieInsertionOptions final {
public:
    /// Get the optional Domain attribute.
    [[nodiscard]] auto domain() const noexcept -> const std::optional<text::String> & { return _domain; }
    /// Set the Domain attribute.
    auto setDomain(text::String value) noexcept -> HttpCookieInsertionOptions & {
        _domain = std::move(value);
        return *this;
    }
    /// Get the optional Path attribute.
    [[nodiscard]] auto path() const noexcept -> const std::optional<text::String> & { return _path; }
    /// Set the Path attribute.
    auto setPath(text::String value) noexcept -> HttpCookieInsertionOptions & {
        _path = std::move(value);
        return *this;
    }
    /// Get the optional absolute expiry.
    [[nodiscard]] auto expires() const noexcept -> const std::optional<time::DateTime> & { return _expires; }
    /// Set the absolute expiry.
    auto setExpires(time::DateTime value) noexcept -> HttpCookieInsertionOptions & {
        _expires = std::move(value);
        return *this;
    }
    /// Get the optional retained SameSite attribute.
    [[nodiscard]] auto sameSite() const noexcept -> std::optional<HttpCookieSameSite> { return _sameSite; }
    /// Set the retained SameSite attribute.
    auto setSameSite(const HttpCookieSameSite value) noexcept -> HttpCookieInsertionOptions & {
        _sameSite = value;
        return *this;
    }
    /// Test whether the Secure attribute is enabled.
    [[nodiscard]] constexpr auto isSecure() const noexcept -> bool { return _secure; }
    /// Enable or disable the Secure attribute.
    constexpr auto setSecure(const bool value) noexcept -> HttpCookieInsertionOptions & {
        _secure = value;
        return *this;
    }
    /// Test whether the HttpOnly attribute is enabled.
    [[nodiscard]] constexpr auto isHttpOnly() const noexcept -> bool { return _httpOnly; }
    /// Enable or disable the HttpOnly attribute.
    constexpr auto setHttpOnly(const bool value) noexcept -> HttpCookieInsertionOptions & {
        _httpOnly = value;
        return *this;
    }

private:
    std::optional<text::String> _domain;         ///< Optional Domain attribute.
    std::optional<text::String> _path;           ///< Optional Path attribute.
    std::optional<time::DateTime> _expires;      ///< Optional expiry.
    std::optional<HttpCookieSameSite> _sameSite; ///< Optional SameSite attribute.
    bool _secure{};                              ///< Secure attribute.
    bool _httpOnly{};                            ///< HttpOnly attribute.
};

}
