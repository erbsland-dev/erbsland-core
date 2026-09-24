// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BitReaderImpl.hpp"

#include "../ByteBlockEditor.hpp"
#include "../SecureErase.hpp"

#include "../../err/OutOfRangeError.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::mem::impl {

BitReaderImpl::BitReaderImpl(ByteBlock data, const std::size_t bitPosition) noexcept :
    _block{std::move(data)}, _data{_block.span()} {
    setBitPosition(bitPosition);
}

BitReaderImpl::~BitReaderImpl() {
    secureErase(std::span{_prefix});
}

void BitReaderImpl::refill(ByteBlock data) {
    if (remainingBitCount() > 64U) {
        throw err::OutOfRangeError{"Bit reader refill requires at most 64 unread bits."};
    }
    const auto begin = bytePosition();
    const auto count = byteCount() - begin;
    for (std::size_t i{}; i < count; ++i) {
        _prefix[i] = byteAt(begin + i);
    }
    _prefixSize = count;
    _bitPosition %= 8U;
    _block = std::move(data);
    _data = _block.span();
}

auto BitReaderImpl::readBytesOrThrow(const unit::ByteLength length) -> ByteBlock {
    if (!isByteAligned() || !length.isFinite() || length.toRawValue() > remainingBitCount() / 8U) {
        throw err::OutOfRangeError{"Byte read requires alignment and sufficient buffered input."};
    }
    const auto count = length.toSizeTOrThrow();
    const auto begin = bytePosition();
    ByteBlock result;
    if (begin >= _prefixSize) {
        result = _block.slice(unit::ByteIndex::fromSizeT(begin - _prefixSize), length);
    } else {
        auto bytes = ByteBlockEditor{};
        bytes.reserve(length);
        bytes.markAsSensitive();
        const auto prefixCount = std::min(count, _prefixSize - begin);
        bytes.append(ConstByteSpan{_prefix}.subspan(begin, prefixCount));
        bytes.append(_data.first(count - prefixCount));
        result = bytes;
    }
    _bitPosition += count * 8U;
    return result;
}

void BitReaderImpl::setBitPosition(const std::size_t bitPosition) noexcept {
    _bitPosition = std::min(bitPosition, bitCount());
}

void BitReaderImpl::setBytePosition(const std::size_t bytePosition) noexcept {
    if (bytePosition >= byteCount()) {
        _bitPosition = bitCount();
        return;
    }
    _bitPosition = bytePosition * 8U;
}

auto BitReaderImpl::canRead(const std::size_t bitCountValue) const noexcept -> bool {
    return bitCountValue <= remainingBitCount();
}

auto BitReaderImpl::canReadBytes(const std::size_t byteCountValue) const noexcept -> bool {
    return byteCountValue <= remainingBitCount() / 8U;
}

void BitReaderImpl::advance(const std::size_t bitCountValue) noexcept {
    _bitPosition += std::min(bitCountValue, remainingBitCount());
}

void BitReaderImpl::advanceBytes(const std::size_t byteCountValue) noexcept {
    if (byteCountValue > remainingBitCount() / 8U) {
        _bitPosition = bitCount();
        return;
    }
    _bitPosition += byteCountValue * 8U;
}

void BitReaderImpl::alignToByte() noexcept {
    advance((8U - _bitPosition % 8U) % 8U);
}

auto BitReaderImpl::readBits(const std::size_t bitCountValue, const uint64_t defaultOnError) noexcept -> uint64_t {
    if (bitCountValue > 64U || !canRead(bitCountValue)) {
        return defaultOnError;
    }
    return readBitsUnchecked(bitCountValue);
}

auto BitReaderImpl::readBitsOrThrow(const std::size_t bitCountValue) -> uint64_t {
    if (bitCountValue > 64U || !canRead(bitCountValue)) {
        throw err::OutOfRangeError{"Bit read exceeds the available data."};
    }
    return readBitsUnchecked(bitCountValue);
}

}
