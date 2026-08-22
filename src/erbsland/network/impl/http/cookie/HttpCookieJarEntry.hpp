// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../text/String.hpp"
#include "../../../../time/DateTime.hpp"
#include "../../../http/HttpCookieSameSite.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::network::impl {

/// Internal storage for one HTTP client cookie.
/// @tested{HttpCookieJarTest HttpClientTest}
struct HttpCookieJarEntry final {
    text::String name;                          ///< Cookie name.
    text::String value;                         ///< Cookie value.
    text::String domain;                        ///< Canonical IDNA ASCII domain.
    text::String path;                          ///< Request path prefix.
    text::String registrableDomain;             ///< Registrable domain used for finite limits.
    std::optional<time::DateTime> expires;      ///< Absolute expiry, if persistent.
    std::optional<HttpCookieSameSite> sameSite; ///< Retained SameSite attribute.
    std::uint64_t created{};                    ///< Stable creation-order sequence.
    std::uint64_t accessed{};                   ///< Last-access sequence for LRU eviction.
    bool hostOnly{};                            ///< Domain attribute was absent.
    bool secure{};                              ///< Secure attribute.
    bool httpOnly{};                            ///< HttpOnly attribute.
};

}
