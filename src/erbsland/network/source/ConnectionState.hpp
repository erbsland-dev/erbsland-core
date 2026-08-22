// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// The lifecycle state of a connected byte-stream source.
enum class ConnectionState : std::uint8_t {
    Inactive,    ///< Configurable and not yet started.
    Connecting,  ///< Resolving or establishing an outgoing transport.
    Accepting,   ///< Adopting an accepted transport.
    Handshaking, ///< Negotiating and authenticating a secure protocol.
    Active,      ///< Application byte-stream traffic is permitted.
    Closing,     ///< Graceful protocol or transport closure is in progress.
    Closed,      ///< Terminal orderly closure or explicit abort.
    Failed,      ///< Terminal state after an operational failure.
};

}
