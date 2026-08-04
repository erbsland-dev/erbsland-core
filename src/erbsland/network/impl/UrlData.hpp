// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HostEndpoint.hpp"
#include "../url/UrlScheme.hpp"

namespace erbsland::network::impl {

/// Shared immutable URL representation.
/// @tested{UrlTest}
class UrlData final {
public:
    UrlScheme scheme{UrlScheme::Invalid}; ///< Parsed scheme.
    text::String schemeText;              ///< Canonical lowercase scheme.
    HostEndpoint endpoint;                ///< Parsed endpoint or default sentinel.
    text::String authorityText;           ///< Original or constructed authority.
    text::String username;                ///< Decoded username.
    text::String password;                ///< Decoded password.
    text::String path;                    ///< Decoded NFC path.
    text::String query;                   ///< Decoded NFC query.
    text::String fragment;                ///< Decoded NFC fragment.
    bool hasAuthority{};                  ///< An authority delimiter was present.
    bool authorityParsed{};               ///< The authority follows the supported host grammar.
    bool hasEndpoint{};                   ///< A usable host endpoint was parsed.
    bool hasUserInfo{};                   ///< A user-info delimiter was present.
    bool hasPassword{};                   ///< A password delimiter was present.
};

}
