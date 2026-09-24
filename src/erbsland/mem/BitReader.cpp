// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BitReader.hpp"

#include "impl/BitReaderImplFor.hpp"

#include <utility>

namespace erbsland::mem {

BitReader::BitReader() : BitReader{ByteBlock{}, BitOrder::MostSignificantFirst} {
}

BitReader::BitReader(ByteBlock data, const BitOrder bitOrder, const std::size_t bitPosition) {
    if (bitOrder == BitOrder::LeastSignificantFirst) {
        _impl = std::make_unique<impl::BitReaderImplFor<BitOrder::LeastSignificantFirst>>(std::move(data), bitPosition);
    } else {
        _impl = std::make_unique<impl::BitReaderImplFor<BitOrder::MostSignificantFirst>>(std::move(data), bitPosition);
    }
}

BitReader::~BitReader() = default;
BitReader::BitReader(BitReader &&) noexcept = default;
auto BitReader::operator=(BitReader &&) noexcept -> BitReader & = default;

void BitReader::refill(ByteBlock data) {
    _impl->refill(std::move(data));
}

auto BitReader::bitOrder() const noexcept -> BitOrder {
    return _impl->bitOrder();
}
auto BitReader::bitCount() const noexcept -> std::size_t {
    return _impl->bitCount();
}
auto BitReader::bitPosition() const noexcept -> std::size_t {
    return _impl->bitPosition();
}
void BitReader::setBitPosition(const std::size_t bitPosition) noexcept {
    _impl->setBitPosition(bitPosition);
}
auto BitReader::byteCount() const noexcept -> std::size_t {
    return _impl->byteCount();
}
auto BitReader::bytePosition() const noexcept -> std::size_t {
    return _impl->bytePosition();
}
void BitReader::setBytePosition(const std::size_t bytePosition) noexcept {
    _impl->setBytePosition(bytePosition);
}
auto BitReader::consumedByteCount() const noexcept -> std::size_t {
    return _impl->consumedByteCount();
}
auto BitReader::remainingBitCount() const noexcept -> std::size_t {
    return _impl->remainingBitCount();
}
auto BitReader::isAtEnd() const noexcept -> bool {
    return _impl->isAtEnd();
}
auto BitReader::isByteAligned() const noexcept -> bool {
    return _impl->isByteAligned();
}
auto BitReader::canRead(const std::size_t bitCountValue) const noexcept -> bool {
    return _impl->canRead(bitCountValue);
}
auto BitReader::canReadBytes(const std::size_t byteCountValue) const noexcept -> bool {
    return _impl->canReadBytes(byteCountValue);
}
void BitReader::advance(const std::size_t bitCountValue) noexcept {
    _impl->advance(bitCountValue);
}
void BitReader::advanceBytes(const std::size_t byteCountValue) noexcept {
    _impl->advanceBytes(byteCountValue);
}
void BitReader::alignToByte() noexcept {
    _impl->alignToByte();
}
auto BitReader::readBits(const std::size_t bitCountValue, const uint64_t defaultOnError) noexcept -> uint64_t {
    return _impl->readBits(bitCountValue, defaultOnError);
}
auto BitReader::readBitsOrThrow(const std::size_t bitCountValue) -> uint64_t {
    return _impl->readBitsOrThrow(bitCountValue);
}
auto BitReader::readBool() noexcept -> bool {
    return readBits(1U) != 0U;
}
auto BitReader::readBoolOrThrow() -> bool {
    return readBitsOrThrow(1U) != 0U;
}
auto BitReader::readByte(const Byte defaultOnError) noexcept -> Byte {
    return Byte{static_cast<uint8_t>(readBits(8U, defaultOnError.toUInt64()))};
}
auto BitReader::readByteOrThrow() -> Byte {
    return Byte{static_cast<uint8_t>(readBitsOrThrow(8U))};
}

auto BitReader::readBytesOrThrow(const unit::ByteLength length) -> ByteBlock {
    return _impl->readBytesOrThrow(length);
}

}
