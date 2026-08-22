// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// The host boundary applied to every redirect relative to the original request URL.
enum class HttpClientRedirectHostPolicy : std::uint8_t {
    Any,                   ///< Permit any HTTP or HTTPS host.
    SameHost,              ///< Require the exact canonical original host.
    SameRegistrableDomain, ///< Require the same registrable domain under the bundled Public Suffix List.
};

}
