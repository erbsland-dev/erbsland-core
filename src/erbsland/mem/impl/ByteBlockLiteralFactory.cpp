// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockLiteralFactory_fwd.hpp"

#include "../ByteBlockLiteral.hpp"

namespace erbsland::mem::impl {

auto unsafeCreateByteBlockLiteral(const std::uint8_t *data, const std::size_t size) noexcept -> ByteBlockLiteral {
    return ByteBlockLiteral{data, size};
}

}
