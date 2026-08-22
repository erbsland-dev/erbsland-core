// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// The action selected for an HTTP client redirect response.
enum class HttpClientRedirectAction : std::uint8_t {
    Follow,         ///< Follow the redirect when every hard guard permits it.
    ReturnResponse, ///< Deliver the redirect response through the ordinary response policy.
    Reject,         ///< Fail the logical request with a redirect-policy error.
};

}
