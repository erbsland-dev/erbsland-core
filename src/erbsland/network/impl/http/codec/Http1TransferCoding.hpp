// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network::impl {

/// The supported result of parsing Transfer-Encoding fields.
enum class Http1TransferCoding : std::uint8_t {
    None,        ///< No Transfer-Encoding field is present.
    Chunked,     ///< Exactly one chunked coding is present.
    Unsupported, ///< A syntactically valid unsupported coding list is present.
};

}
