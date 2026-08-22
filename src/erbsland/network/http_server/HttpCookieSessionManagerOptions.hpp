// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpCookieSecurePolicy.hpp"

#include "../http/HttpCookieSameSite.hpp"

#include "../../text/String.hpp"
#include "../../time/TimeDelta.hpp"
#include "../../unit/ItemCount.hpp"

#include <utility>

namespace erbsland::network {

/// Configuration for the bounded cookie-backed server session manager.
/// @tested{HttpCookieSessionManagerTest}
class HttpCookieSessionManagerOptions final {
public:
    /// Default sliding inactivity lifetime.
    inline static const auto cDefaultIdleTimeout = time::TimeDelta::minutes(30);
    /// Default total session lifetime.
    inline static const auto cDefaultAbsoluteTimeout = time::TimeDelta::hours(24);
    /// Default bounded registry capacity.
    static constexpr auto cDefaultMaximumSessions = unit::ItemCount{10'000U};

public:
    /// Get the manager-reserved cookie name.
    [[nodiscard]] auto cookieName() const noexcept -> const text::String & { return _cookieName; }
    /// Set the manager-reserved cookie name.
    auto setCookieName(text::String value) noexcept -> HttpCookieSessionManagerOptions & {
        _cookieName = std::move(value);
        return *this;
    }
    /// Get the cookie Path attribute.
    [[nodiscard]] auto cookiePath() const noexcept -> const text::String & { return _cookiePath; }
    /// Set the cookie Path attribute.
    auto setCookiePath(text::String value) noexcept -> HttpCookieSessionManagerOptions & {
        _cookiePath = std::move(value);
        return *this;
    }
    /// Get the SameSite policy.
    [[nodiscard]] auto sameSite() const noexcept -> HttpCookieSameSite { return _sameSite; }
    /// Set the SameSite policy.
    auto setSameSite(HttpCookieSameSite value) noexcept -> HttpCookieSessionManagerOptions & {
        _sameSite = value;
        return *this;
    }
    /// Get the Secure-attribute policy.
    [[nodiscard]] auto securePolicy() const noexcept -> HttpCookieSecurePolicy { return _securePolicy; }
    /// Set the Secure-attribute policy.
    auto setSecurePolicy(HttpCookieSecurePolicy value) noexcept -> HttpCookieSessionManagerOptions & {
        _securePolicy = value;
        return *this;
    }
    /// Get the sliding inactivity lifetime.
    [[nodiscard]] auto idleTimeout() const noexcept -> time::TimeDelta { return _idleTimeout; }
    /// Set the positive sliding inactivity lifetime.
    auto setIdleTimeout(time::TimeDelta value) noexcept -> HttpCookieSessionManagerOptions & {
        _idleTimeout = value;
        return *this;
    }
    /// Get the total session lifetime.
    [[nodiscard]] auto absoluteTimeout() const noexcept -> time::TimeDelta { return _absoluteTimeout; }
    /// Set the positive total session lifetime.
    auto setAbsoluteTimeout(time::TimeDelta value) noexcept -> HttpCookieSessionManagerOptions & {
        _absoluteTimeout = value;
        return *this;
    }
    /// Get the finite server-side registry capacity.
    [[nodiscard]] auto maximumSessions() const noexcept -> unit::ItemCount { return _maximumSessions; }
    /// Set the finite server-side registry capacity.
    auto setMaximumSessions(unit::ItemCount value) noexcept -> HttpCookieSessionManagerOptions & {
        _maximumSessions = value;
        return *this;
    }

private:
    text::String _cookieName{"erbsland-session"};                            ///< Reserved cookie name.
    text::String _cookiePath{"/"};                                           ///< Cookie path.
    HttpCookieSameSite _sameSite{HttpCookieSameSite::Lax};                   ///< SameSite attribute.
    HttpCookieSecurePolicy _securePolicy{HttpCookieSecurePolicy::Automatic}; ///< Secure attribute policy.
    time::TimeDelta _idleTimeout{cDefaultIdleTimeout};                       ///< Sliding inactivity limit.
    time::TimeDelta _absoluteTimeout{cDefaultAbsoluteTimeout};               ///< Maximum total lifetime.
    unit::ItemCount _maximumSessions{cDefaultMaximumSessions};               ///< Registry capacity.
};

}
