// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BitWriter.hpp"

#include "impl/BitWriterImplFor.hpp"

namespace erbsland::mem {

BitWriter::BitWriter(const BitOrder bitOrder) {
    if (bitOrder == BitOrder::LeastSignificantFirst) {
        _impl = std::make_unique<impl::BitWriterImplFor<BitOrder::LeastSignificantFirst>>();
    } else {
        _impl = std::make_unique<impl::BitWriterImplFor<BitOrder::MostSignificantFirst>>();
    }
}

BitWriter::~BitWriter() = default;
BitWriter::BitWriter(BitWriter &&) noexcept = default;
auto BitWriter::operator=(BitWriter &&) noexcept -> BitWriter & = default;

auto BitWriter::bitOrder() const noexcept -> BitOrder {
    return _impl->bitOrder();
}
auto BitWriter::bitCount() const noexcept -> std::size_t {
    return _impl->bitCount();
}
auto BitWriter::bitPosition() const noexcept -> std::size_t {
    return _impl->bitPosition();
}
void BitWriter::setBitPosition(const std::size_t bitPosition) noexcept {
    _impl->setBitPosition(bitPosition);
}
auto BitWriter::byteCount() const noexcept -> std::size_t {
    return _impl->byteCount();
}
auto BitWriter::bytePosition() const noexcept -> std::size_t {
    return _impl->bytePosition();
}
void BitWriter::setBytePosition(const std::size_t bytePosition) noexcept {
    _impl->setBytePosition(bytePosition);
}
auto BitWriter::consumedByteCount() const noexcept -> std::size_t {
    return _impl->consumedByteCount();
}
auto BitWriter::remainingBitCount() const noexcept -> std::size_t {
    return _impl->remainingBitCount();
}
auto BitWriter::isAtEnd() const noexcept -> bool {
    return _impl->isAtEnd();
}
auto BitWriter::isByteAligned() const noexcept -> bool {
    return _impl->isByteAligned();
}
void BitWriter::advance(const std::size_t bitCountValue) noexcept {
    _impl->advance(bitCountValue);
}
void BitWriter::advanceBytes(const std::size_t byteCountValue) noexcept {
    _impl->advanceBytes(byteCountValue);
}
auto BitWriter::alignToByte() -> BitWriter & {
    _impl->alignToByte();
    return *this;
}
void BitWriter::markAsSensitive() noexcept {
    _impl->markAsSensitive();
}
void BitWriter::reset() noexcept {
    _impl->reset();
}
auto BitWriter::reserveBits(const std::size_t bitCapacity) -> BitWriter & {
    _impl->reserveBits(bitCapacity);
    return *this;
}
auto BitWriter::reserveBytes(const std::size_t byteCapacity) -> BitWriter & {
    _impl->reserveBytes(byteCapacity);
    return *this;
}
auto BitWriter::writeBits(const uint64_t value, const std::size_t bitCountValue) -> BitWriter & {
    _impl->writeBits(value, bitCountValue);
    return *this;
}
auto BitWriter::writeBool(const bool value) -> BitWriter & {
    return writeBits(value ? 1U : 0U, 1U);
}
auto BitWriter::writeByte(const Byte value) -> BitWriter & {
    return writeBits(value.toUInt64(), 8U);
}
auto BitWriter::writeBytes(const ConstByteSpan bytes) -> BitWriter & {
    _impl->writeBytes(bytes);
    return *this;
}
auto BitWriter::toByteBlock() const noexcept -> ByteBlock {
    return _impl->toByteBlock();
}
auto BitWriter::takeByteBlockEditor() noexcept -> ByteBlockEditor {
    return _impl->takeByteBlockEditor();
}

}
