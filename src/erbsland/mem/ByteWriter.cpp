// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteWriter.hpp"

namespace erbsland::mem {

void ByteWriter::setPosition(const unit::ByteIndex position) noexcept {
    if (position.isNoIndex() || position > unit::ByteIndex::end(length())) {
        _position = unit::ByteIndex::end(length());
        return;
    }
    _position = position;
}

auto ByteWriter::reserve(const unit::ByteLength capacity) -> ByteWriter & {
    _block.reserve(capacity);
    return *this;
}

auto ByteWriter::writeByte(const Byte value) -> ByteWriter & {
    if (_position >= unit::ByteIndex::end(length())) {
        _block.append(value);
    } else {
        _block.set(_position, value);
    }
    setPosition(_position + unit::ByteLength::one());
    return *this;
}

}
