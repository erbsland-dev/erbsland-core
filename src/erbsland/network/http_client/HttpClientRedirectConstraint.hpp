// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// A machine-readable hard guard that prevents following a redirect.
enum class HttpClientRedirectConstraint : std::uint8_t {
    None,              ///< The redirect is eligible to be followed.
    InvalidTarget,     ///< Location is absent, malformed, or not HTTP(S).
    RedirectLimit,     ///< The captured redirect limit would be exceeded.
    Loop,              ///< The canonical fragment-free target was already requested.
    HttpsDowngrade,    ///< HTTPS-to-HTTP downgrade is disabled.
    HostPolicy,        ///< The target violates the captured host policy.
    BodyNotReplayable, ///< The redirect must preserve a streamed request body.
};

}
