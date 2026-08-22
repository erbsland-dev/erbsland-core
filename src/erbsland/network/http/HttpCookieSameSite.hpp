// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// SameSite policy for the built-in HTTP session cookie.
enum class HttpCookieSameSite : uint8_t {
    Strict, ///< Send only in same-site contexts.
    Lax,    ///< Permit safe top-level cross-site navigation.
    None,   ///< Permit cross-site use; requires a secure cookie.
};

}
