// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRedirectAction.hpp"
#include "HttpClientRedirectHostPolicy.hpp"

#include "../../unit/ItemCount.hpp"

namespace erbsland::network {

/// Redirect policy captured when an HTTP client request is submitted.
/// @tested{HttpClientTest}
class HttpClientRedirectOptions final {
public:
    static constexpr auto cDefaultMaximumRedirects = unit::ItemCount{10U}; ///< Default followed-hop limit.

public:
    /// Get the action used when no redirect callback is installed.
    [[nodiscard]] constexpr auto action() const noexcept -> HttpClientRedirectAction { return _action; }
    /// Set the action used when no redirect callback is installed.
    constexpr auto setAction(const HttpClientRedirectAction value) noexcept -> HttpClientRedirectOptions & {
        _action = value;
        return *this;
    }
    /// Get the maximum number of followed redirects.
    [[nodiscard]] constexpr auto maximumRedirects() const noexcept -> unit::ItemCount { return _maximumRedirects; }
    /// Set the finite maximum number of followed redirects.
    constexpr auto setMaximumRedirects(const unit::ItemCount value) noexcept -> HttpClientRedirectOptions & {
        _maximumRedirects = value;
        return *this;
    }
    /// Get the redirect host policy.
    [[nodiscard]] constexpr auto hostPolicy() const noexcept -> HttpClientRedirectHostPolicy { return _hostPolicy; }
    /// Set the redirect host policy.
    constexpr auto setHostPolicy(const HttpClientRedirectHostPolicy value) noexcept -> HttpClientRedirectOptions & {
        _hostPolicy = value;
        return *this;
    }
    /// Test whether HTTPS-to-HTTP redirects may be followed.
    [[nodiscard]] constexpr auto allowsHttpsDowngrade() const noexcept -> bool { return _allowHttpsDowngrade; }
    /// Permit or reject HTTPS-to-HTTP redirects.
    constexpr auto setAllowHttpsDowngrade(const bool value) noexcept -> HttpClientRedirectOptions & {
        _allowHttpsDowngrade = value;
        return *this;
    }

private:
    HttpClientRedirectAction _action{HttpClientRedirectAction::Follow};          ///< Default action.
    unit::ItemCount _maximumRedirects{cDefaultMaximumRedirects};                 ///< Followed-hop limit.
    HttpClientRedirectHostPolicy _hostPolicy{HttpClientRedirectHostPolicy::Any}; ///< Original-host boundary.
    bool _allowHttpsDowngrade{};                                                 ///< Secure downgrade policy.
};

}
