// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteOutputStream.hpp"

#include <vector>

namespace erbsland::stream {

auto ByteOutputStream::endianness() const noexcept -> mem::Endianness {
    return _endianness;
}

void ByteOutputStream::setEndianness(const mem::Endianness endianness) noexcept {
    _endianness = endianness;
}

void ByteOutputStream::write(const mem::Byte byte) {
    write(std::span<const mem::Byte>{&byte, 1U});
}

void ByteOutputStream::write(const mem::ByteBlockView &bytes) {
    const auto vector = bytes.toByteVector();
    write(std::span<const mem::Byte>{vector});
}

}
