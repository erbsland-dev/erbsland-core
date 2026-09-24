// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BitWriterImpl.hpp"

#include "../../err/OutOfRangeError.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace erbsland::mem::impl {

void BitWriterImpl::setBitPosition(const std::size_t bitPosition) noexcept {
    _bitPosition = std::min(bitPosition, _bitCount);
}

void BitWriterImpl::setBytePosition(const std::size_t bytePosition) noexcept {
    if (bytePosition > _bitCount / 8U) {
        _bitPosition = _bitCount;
        return;
    }
    setBitPosition(bytePosition * 8U);
}

void BitWriterImpl::advance(const std::size_t bitCountValue) noexcept {
    _bitPosition += std::min(bitCountValue, remainingBitCount());
}

void BitWriterImpl::advanceBytes(const std::size_t byteCountValue) noexcept {
    if (byteCountValue > remainingBitCount() / 8U) {
        _bitPosition = _bitCount;
        return;
    }
    _bitPosition += byteCountValue * 8U;
}

void BitWriterImpl::alignToByte() {
    writeBits(0U, (8U - _bitPosition % 8U) % 8U);
}

void BitWriterImpl::reset() noexcept {
    _block.reset();
    _bitCount = 0U;
    _bitPosition = 0U;
}

void BitWriterImpl::reserveBits(const std::size_t bitCapacity) {
    reserveBytes(byteCountForBits(bitCapacity));
}

void BitWriterImpl::reserveBytes(const std::size_t byteCapacity) {
    _block.reserve(unit::ByteLength::fromSizeT(byteCapacity));
    if (_sensitive) {
        _block.markAsSensitive();
    }
}

void BitWriterImpl::prepareWrite(const std::size_t bitCountValue) {
    if (bitCountValue > std::numeric_limits<std::size_t>::max() - _bitPosition) {
        throw err::OutOfRangeError{"Bit write position overflow."};
    }
    const auto requiredByteCount = byteCountForBits(_bitPosition + bitCountValue);
    if (requiredByteCount > _block.length().toSizeT()) {
        _block.resize(unit::ByteLength::fromSizeT(requiredByteCount));
        if (_sensitive) {
            _block.markAsSensitive();
        }
    }
}

void BitWriterImpl::writeBits(const uint64_t value, const std::size_t bitCountValue) {
    if (bitCountValue > 64U) {
        throw err::OutOfRangeError{"Bit write exceeds 64 bits."};
    }
    const auto endPosition = writeBitsUnchecked(value, bitCountValue);
    _bitPosition = endPosition;
    _bitCount = std::max(_bitCount, endPosition);
}

void BitWriterImpl::writeBytes(const ConstByteSpan bytes) {
    if (!isByteAligned() || bytes.size() > (std::numeric_limits<std::size_t>::max() - _bitPosition) / 8U) {
        throw err::OutOfRangeError{"Byte write requires alignment and an addressable bit position."};
    }
    _block.replace(
        unit::ByteRange{unit::ByteIndex::fromSizeT(bytePosition()), unit::ByteLength::fromSizeT(bytes.size())}, bytes);
    if (_sensitive) {
        _block.markAsSensitive();
    }
    _bitPosition += bytes.size() * 8U;
    _bitCount = std::max(_bitCount, _bitPosition);
}

auto BitWriterImpl::toByteBlock() const noexcept -> ByteBlock {
    return ByteBlock{_block};
}

auto BitWriterImpl::takeByteBlockEditor() noexcept -> ByteBlockEditor {
    auto result = std::move(_block);
    _block = ByteBlockEditor{};
    _bitCount = 0U;
    _bitPosition = 0U;
    return result;
}

}
