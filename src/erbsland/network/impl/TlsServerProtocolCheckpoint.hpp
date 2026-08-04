// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network::impl {

/// Ordered policy checkpoints of the internal TLS 1.3 server core.
/// Specification: RFC 8446 Sections 4.1.2 and 4.4.4.
enum class TlsServerProtocolCheckpoint : uint8_t {
    None,               ///< Protocol processing is not paused.
    ClientHello,        ///< ClientHello is validated; no server secret or output exists.
    HandshakeCompleted, ///< Client Finished is authenticated; application delivery remains paused.
};

}
