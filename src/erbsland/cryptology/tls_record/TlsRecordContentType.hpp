// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// An authenticated TLS 1.3 inner-plaintext content type from RFC 8446 sections 5.1 and 5.2.
enum class TlsRecordContentType : uint8_t {
    Alert = 21U,           ///< One TLS alert message.
    Handshake = 22U,       ///< One or more TLS handshake-message fragments.
    ApplicationData = 23U, ///< Opaque application data.
};

}
