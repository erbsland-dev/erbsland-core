// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network::impl {

/// The HTTP/1.1 message-body framing selected from control data and fields.
enum class Http1BodyFraming : std::uint8_t {
    None,           ///< The message has no body.
    FixedLength,    ///< Content-Length delimits the body.
    Chunked,        ///< Chunked transfer coding delimits and decodes the body.
    CloseDelimited, ///< Transport EOF delimits a response body.
    Opaque,         ///< The connection switched to a tunnel or another protocol.
};

}
