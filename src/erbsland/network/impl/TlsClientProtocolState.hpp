// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network::impl {

/// Observable lifecycle of the internal transport-independent TLS 1.3 client.
enum class TlsClientProtocolState : uint8_t {
    Inactive,    ///< No ClientHello has been created.
    Handshaking, ///< The authenticated TLS 1.3 handshake is in progress.
    Established, ///< Application data can be sent and received.
    Closing,     ///< Local or peer close_notify was processed, with output still pending.
    Closed,      ///< Bidirectional close_notify completed or transport ended after orderly peer closure.
    Failed,      ///< A terminal protocol, authentication, timeout, or truncation failure occurred.
};

}
