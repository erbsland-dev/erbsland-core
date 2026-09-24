// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteBlockLiteral_fwd.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::mem::impl {

/// Unsafely create a byte block literal from trusted static unsigned-byte storage.
/// The caller must guarantee that the storage remains valid for the lifetime of every resulting literal and block.
/// @tested{ByteBlockLiteralTest}
[[nodiscard]] auto unsafeCreateByteBlockLiteral(const std::uint8_t *data, std::size_t size) noexcept
    -> ByteBlockLiteral;

}
