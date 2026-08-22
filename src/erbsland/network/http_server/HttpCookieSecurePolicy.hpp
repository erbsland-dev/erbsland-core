// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// Secure-attribute policy for the built-in HTTP session cookie.
enum class HttpCookieSecurePolicy : uint8_t {
    Automatic, ///< Add Secure for HTTPS requests.
    Always,    ///< Always add Secure.
    Never,     ///< Never add Secure.
};

}
