// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network::impl {

/// Lifecycle state of the transport-independent TLS 1.3 server core.
/// Specification: RFC 8446 Appendix A.2.
enum class TlsServerProtocolState : uint8_t {
    Inactive,    ///< No ClientHello may be processed yet.
    Handshaking, ///< The authenticated TLS 1.3 handshake is incomplete.
    Established, ///< Client Finished was authenticated.
    Closing,     ///< An orderly close is waiting for peer or output drain.
    Closed,      ///< The protocol closed without a terminal failure.
    Failed,      ///< A terminal protocol or local failure occurred.
};

}
