// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteReader.hpp"

namespace erbsland::mem {

ByteReader::ByteReader(const ByteBlock &block) noexcept : _view{block} {
}

ByteReader::ByteReader(const ByteBlockView &view) noexcept : _view{view} {
}

void ByteReader::setPosition(const unit::ByteIndex position) noexcept {
    if (position.isNoIndex() || position > unit::ByteIndex::end(length())) {
        _position = unit::ByteIndex::end(length());
        return;
    }
    _position = position;
}

auto ByteReader::readByte() noexcept -> Byte {
    if (!canRead(1U)) {
        return {};
    }
    auto result = _view.get(_position);
    advance(1U);
    return result;
}

auto ByteReader::peekByte() const noexcept -> Byte {
    return peekByte(0U);
}

auto ByteReader::peekByte(const std::size_t offset, const Byte defaultValue) const noexcept -> Byte {
    if (_position.isNoIndex()) {
        return defaultValue;
    }
    return _view.get(_position + unit::ByteLength::fromSizeT(offset), defaultValue);
}

auto ByteReader::readByteOrThrow() -> Byte {
    auto result = peekByteOrThrow();
    advance(1U);
    return result;
}

auto ByteReader::peekByteOrThrow() const -> Byte {
    return _view.getOrThrow(_position);
}

auto ByteReader::canRead(const std::size_t byteCount) const noexcept -> bool {
    if (_position.isNoIndex()) {
        return false;
    }
    const auto currentPosition = _position.toSizeT();
    const auto currentLength = length().toSizeT();
    return currentPosition <= currentLength && byteCount <= currentLength - currentPosition;
}

void ByteReader::advance(const std::size_t byteCount) noexcept {
    setPosition(_position + unit::ByteLength::fromSizeT(byteCount));
}

}
