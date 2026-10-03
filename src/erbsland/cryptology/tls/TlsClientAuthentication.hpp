// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// Whether a TLS server requests and requires a client certificate.
enum class TlsClientAuthentication : uint8_t {
    Disabled, ///< Do not request a certificate.
    Optional, ///< Accept absence, but verify every presented certificate.
    Required, ///< Require a verified certificate and proof of key possession.
};

}
