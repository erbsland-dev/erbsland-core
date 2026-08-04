// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// The operation phase in which a network failure occurred.
enum class NetworkErrorPhase : uint8_t {
    None,          ///< No more specific phase is available.
    Configuration, ///< Synchronous connection configuration.
    Accepting,     ///< Incoming connection admission or transport adoption.
    Resolving,     ///< Host-name resolution.
    Connecting,    ///< Transport connection establishment.
    Handshaking,   ///< TLS protocol negotiation and peer authentication.
    Active,        ///< Authenticated application traffic.
    Closing,       ///< Graceful protocol or transport closure.
};

}
