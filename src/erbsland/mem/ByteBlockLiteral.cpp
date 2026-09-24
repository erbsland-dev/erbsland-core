// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockLiteral.hpp"

namespace erbsland::mem {

auto ByteBlockLiteral::span() const noexcept -> ConstByteSpan {
    if (_data == nullptr || _size == 0U) {
        return {};
    }
    return ConstByteSpan{static_cast<const Byte *>(_data), _size};
}

}
