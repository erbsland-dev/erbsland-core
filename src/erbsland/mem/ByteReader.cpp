// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteReader.hpp"

#include "impl/ByteReaderTools.hpp"
#include "impl/ByteReadTools.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../text/String.hpp"

namespace erbsland::mem {

using namespace text::literals;

ByteReader::ByteReader(const ByteBlockEditor &editor) noexcept : _block{editor} {
}

ByteReader::ByteReader(const ByteBlock &block) noexcept : _block{block} {
}

auto ByteReader::length() const noexcept -> unit::ByteLength {
    return impl::ByteReadTools{dataView()}.length();
}

void ByteReader::setPosition(const unit::ByteIndex position) noexcept {
    if (position.isNoIndex() || position > unit::ByteIndex::end(length())) {
        _position = unit::ByteIndex::end(length());
        return;
    }
    _position = position;
}

auto ByteReader::readByte() noexcept -> Byte {
    auto tools = impl::ByteReadTools{dataView()};
    if (_position >= tools.endIndex()) {
        return {};
    }
    const auto result = tools.get(_position);
    advance(1U);
    return result;
}

auto ByteReader::peekByte() const noexcept -> Byte {
    return peekByte(0U);
}

auto ByteReader::peekByte(const unit::ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    return impl::ByteReadTools{dataView()}.get(index, defaultValue);
}

auto ByteReader::peekByte(const std::size_t offset, const Byte defaultValue) const noexcept -> Byte {
    if (_position.isNoIndex()) {
        return defaultValue;
    }
    return impl::ByteReadTools{dataView()}.get(_position + unit::ByteLength::fromSizeT(offset), defaultValue);
}

auto ByteReader::readByteOrThrow() -> Byte {
    auto result = peekByteOrThrow();
    advance(1U);
    return result;
}

auto ByteReader::peekByteOrThrow() const -> Byte {
    return impl::ByteReadTools{dataView()}.getOrThrow(_position);
}

auto ByteReader::readBytes(const unit::ByteLength lengthValue) noexcept -> std::optional<ByteBlock> {
    try {
        return readBytesOrThrow(lengthValue);
    } catch (...) {
        return std::nullopt;
    }
}

auto ByteReader::readBytesOrThrow(const unit::ByteLength lengthValue) -> ByteBlock {
    if (!lengthValue.isFinite() || !canRead(lengthValue)) {
        throw err::OutOfRangeError("Not enough bytes to read the requested byte block"_el);
    }
    const auto result = _block.slice(_position, lengthValue);
    advance(lengthValue);
    return result;
}

auto ByteReader::readText(const ByteTextOptions &options) -> std::optional<text::String> {
    try {
        return readTextOrThrow(options);
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

auto ByteReader::readTextOrThrow(const ByteTextOptions &options) -> text::String {
    return impl::ByteReaderTools{*this}.readTextOrThrow(options);
}

auto ByteReader::canRead(const unit::ByteLength byteCount) const noexcept -> bool {
    if (_position.isNoIndex()) {
        return false;
    }
    const auto currentPosition = _position.toSizeT();
    const auto currentLength = length().toSizeT();
    return byteCount.isFinite() && currentPosition <= currentLength &&
        byteCount.toSizeT() <= currentLength - currentPosition;
}

void ByteReader::advance(const unit::ByteLength byteCount) noexcept {
    setPosition(_position + byteCount);
}

auto ByteReader::dataView() const noexcept -> impl::ByteDataView {
    return impl::ByteDataView{_block.span()};
}

}
