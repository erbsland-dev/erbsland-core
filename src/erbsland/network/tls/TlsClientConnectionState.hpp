// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// Lifecycle state of a one-shot TLS client connection.
enum class TlsClientConnectionState : uint8_t {
    Inactive,    ///< Configurable and not started.
    Connecting,  ///< Resolving or connecting the TCP transport.
    Handshaking, ///< Negotiating TLS and authenticating the peer.
    Active,      ///< Authenticated application traffic is permitted.
    Closing,     ///< Bidirectional TLS closure is in progress.
    Closed,      ///< Terminal orderly closure or explicit abort.
    Failed,      ///< Terminal operational failure.
};

}
