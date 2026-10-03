// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteReader.hpp"

#include "impl/ByteReaderTools.hpp"
#include "impl/ByteReadTools.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../err/OutOfRangeFailure.hpp"
#include "../err/RuntimeError.hpp"
#include "../text/String.hpp"

namespace erbsland::mem {

using namespace unit;
using namespace text::literals;

ByteReader::ByteReader(const ByteBlockEditor &editor) noexcept : _block{editor} {
}

ByteReader::ByteReader(const ByteBlock &block) noexcept : _block{block} {
}

auto ByteReader::length() const noexcept -> ByteCount {
    return impl::ByteReadTools{dataView()}.length();
}

void ByteReader::setPosition(const ByteIndex position) noexcept {
    if (position.isNoIndex() || position > ByteIndex::end(length())) {
        _position = ByteIndex::end(length());
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

auto ByteReader::peekByte(const ByteIndex index, const Byte defaultValue) const noexcept -> Byte {
    return impl::ByteReadTools{dataView()}.get(index, defaultValue);
}

auto ByteReader::peekByte(const std::size_t offset, const Byte defaultValue) const noexcept -> Byte {
    if (_position.isNoIndex()) {
        return defaultValue;
    }
    return impl::ByteReadTools{dataView()}.get(_position + ByteCount::fromSizeT(offset), defaultValue);
}

auto ByteReader::readByteOrThrow() -> Byte {
    auto result = peekByteOrThrow();
    advance(1U);
    return result;
}

auto ByteReader::peekByteOrThrow() const -> Byte {
    return impl::ByteReadTools{dataView()}.getOrThrow(_position);
}

auto ByteReader::readBytes(const ByteCount lengthValue) noexcept -> std::optional<ByteBlock> {
    if (!canRead(lengthValue)) {
        return std::nullopt;
    }
    return readBytesOrThrow(lengthValue);
}

auto ByteReader::readBytesOrThrow(const ByteCount lengthValue) -> ByteBlock {
    if (!lengthValue.isFinite()) {
        throw err::OutOfRangeFailure{"Byte read requires a finite length"_el};
    }
    if (!canRead(lengthValue)) {
        throw err::OutOfRangeError("Not enough bytes to read the requested byte block"_el);
    }
    const auto result = _block.slice(_position, lengthValue);
    advance(lengthValue);
    return result;
}

auto ByteReader::readText(const ByteTextOptions &options) -> std::optional<text::String> {
    try {
        return readTextOrThrow(options);
    } catch (const err::RuntimeError &) {
        return std::nullopt;
    }
}

auto ByteReader::readTextOrThrow(const ByteTextOptions &options) -> text::String {
    return impl::ByteReaderTools{*this}.readTextOrThrow(options);
}

auto ByteReader::canRead(const ByteCount byteCount) const noexcept -> bool {
    if (_position.isNoIndex()) {
        return false;
    }
    const auto currentPosition = _position.toSizeT();
    const auto currentLength = length().toSizeT();
    return byteCount.isFinite() && currentPosition <= currentLength &&
        byteCount.toSizeT() <= currentLength - currentPosition;
}

void ByteReader::advance(const ByteCount byteCount) noexcept {
    setPosition(_position + byteCount);
}

auto ByteReader::dataView() const noexcept -> impl::ByteDataView {
    return impl::ByteDataView{_block.span()};
}

}
