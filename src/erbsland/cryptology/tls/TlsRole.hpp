// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// The local role used to select a TLS profile.
enum class TlsRole : uint8_t {
    Client, ///< Initiate a connection and authenticate a server.
    Server, ///< Accept a connection and optionally authenticate a client.
};

}
