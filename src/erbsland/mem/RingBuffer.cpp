// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RingBuffer.hpp"

#include "SecureErase.hpp"

#include "impl/BestGrowth.hpp"
#include "impl/RingBufferStorageTraits.hpp"
#include "impl/UnsafeByteBlockBuffer.hpp"

#include "../err/LogicError.hpp"
#include "../err/ParameterError.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <utility>

namespace erbsland::mem {

using unit::ByteLength;

RingBuffer::RingBuffer(const ByteLength capacity) : RingBuffer{capacity, capacity} {
}

RingBuffer::RingBuffer(const ByteLength initialCapacity, const ByteLength maximumCapacity) :
    _storage(initialCapacity.toSizeTOrThrow()),
    _initialCapacity{initialCapacity.toSizeTOrThrow()},
    _maximumCapacity{maximumCapacity.toSizeTOrThrow()} {
    if (_initialCapacity == 0U) {
        throw err::ParameterError{"Ring buffer capacity must not be zero.", "initialCapacity"};
    }
    if (_maximumCapacity < _initialCapacity) {
        throw err::ParameterError{
            "Ring buffer maximum capacity is smaller than its initial capacity.", "maximumCapacity"};
    }
}

RingBuffer::~RingBuffer() {
    if (_sensitive) {
        secureErase();
    }
}

auto RingBuffer::capacity() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(_storage.size());
}

auto RingBuffer::maximumCapacity() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(_maximumCapacity);
}

auto RingBuffer::length() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(_length);
}

auto RingBuffer::available() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(_storage.size() - _length);
}

auto RingBuffer::canWrite(const ByteLength length) const noexcept -> bool {
    return !length.isInfinite() && length.toSizeT() <= _maximumCapacity - _length;
}

void RingBuffer::setSensitive(const bool sensitive) noexcept {
    if (_unsafeAccessActive) {
        std::terminate();
    }
    if (_sensitive && !sensitive) {
        secureErase();
    }
    _sensitive = sensitive;
}

auto RingBuffer::reserveAdditional(const ByteLength length) -> util::Result {
    verifySafeAccess();
    if (!canWrite(length)) {
        return util::Result::Failure;
    }
    const auto required = _length + length.toSizeT();
    if (required <= _storage.size()) {
        return util::Result::Success;
    }

    const auto bestCapacity = impl::bestGrowthCapacity<impl::RingBufferStorageTraits>(
        _storage.size(), required, impl::BestGrowthStrategy::Geometric);
    const auto newCapacity = std::min(bestCapacity, _maximumCapacity);
    auto newStorage = std::vector<Byte>(newCapacity);
    const auto readable = readableSpans();
    auto offset = std::size_t{0};
    for (const auto span : readable) {
        if (!span.empty()) {
            std::memcpy(newStorage.data() + offset, span.data(), span.size() * sizeof(Byte));
            offset += span.size();
        }
    }
    _storage.swap(newStorage);
    if (_sensitive) {
        mem::secureErase(ByteSpan{newStorage});
    }
    _readPosition = 0U;
    return util::Result::Success;
}

auto RingBuffer::write(const ConstByteSpan bytes) -> ByteLength {
    verifySafeAccess();
    const auto maximumWrite = std::min(bytes.size(), _maximumCapacity - _length);
    if (isFailure(reserveAdditional(ByteLength::fromSizeT(maximumWrite)))) {
        return ByteLength::zero();
    }
    auto remaining = maximumWrite;
    auto sourceOffset = std::size_t{0};
    for (const auto destination : writableSpans()) {
        const auto count = std::min(destination.size(), remaining);
        if (count > 0U) {
            std::memcpy(destination.data(), bytes.data() + sourceOffset, count * sizeof(Byte));
            sourceOffset += count;
            remaining -= count;
        }
    }
    commitWritten(maximumWrite);
    return ByteLength::fromSizeT(maximumWrite);
}

auto RingBuffer::writeExact(const ConstByteSpan bytes) -> util::Result {
    verifySafeAccess();
    if (isFailure(reserveAdditional(ByteLength::fromSizeT(bytes.size())))) {
        return util::Result::Failure;
    }
    return write(bytes) == ByteLength::fromSizeT(bytes.size()) ? util::Result::Success : util::Result::Failure;
}

auto RingBuffer::read(const ByteSpan destination) -> ByteLength {
    verifySafeAccess();
    const auto maximumRead = std::min(destination.size(), _length);
    auto remaining = maximumRead;
    auto destinationOffset = std::size_t{0};
    for (const auto source : readableSpans()) {
        const auto count = std::min(source.size(), remaining);
        if (count > 0U) {
            std::memcpy(destination.data() + destinationOffset, source.data(), count * sizeof(Byte));
            destinationOffset += count;
            remaining -= count;
        }
    }
    consumeRead(maximumRead);
    return ByteLength::fromSizeT(maximumRead);
}

auto RingBuffer::read(const ByteLength maximum) -> ByteBlock {
    verifySafeAccess();
    const auto readLength = maximum.isInfinite() ? _length : std::min(maximum.toSizeT(), _length);
    auto buffer = impl::UnsafeByteBlockBuffer{ByteLength::fromSizeT(readLength), _sensitive};
    static_cast<void>(read(buffer.data()));
    return buffer.take(ByteLength::fromSizeT(readLength));
}

void RingBuffer::clear() noexcept {
    if (_unsafeAccessActive) {
        std::terminate();
    }
    if (_sensitive) {
        secureErase();
    } else {
        _readPosition = 0U;
        _length = 0U;
    }
}

void RingBuffer::secureErase() noexcept {
    if (_unsafeAccessActive) {
        std::terminate();
    }
    mem::secureErase(ByteSpan{_storage});
    _readPosition = 0U;
    _length = 0U;
}

void RingBuffer::shrinkToInitial() {
    verifySafeAccess();
    if (!isEmpty() || _storage.size() == _initialCapacity) {
        return;
    }
    auto newStorage = std::vector<Byte>(_initialCapacity);
    _storage.swap(newStorage);
    if (_sensitive) {
        mem::secureErase(ByteSpan{newStorage});
    }
    _readPosition = 0U;
}

void RingBuffer::swap(RingBuffer &other) noexcept {
    if (_unsafeAccessActive || other._unsafeAccessActive) {
        std::terminate();
    }
    using std::swap;
    swap(_storage, other._storage);
    swap(_initialCapacity, other._initialCapacity);
    swap(_maximumCapacity, other._maximumCapacity);
    swap(_readPosition, other._readPosition);
    swap(_length, other._length);
    swap(_sensitive, other._sensitive);
}

auto RingBuffer::readableSpans() const noexcept -> std::array<ConstByteSpan, 2> {
    if (_length == 0U) {
        return {};
    }
    const auto firstLength = std::min(_length, _storage.size() - _readPosition);
    const auto secondLength = _length - firstLength;
    return {ConstByteSpan{_storage.data() + _readPosition, firstLength}, ConstByteSpan{_storage.data(), secondLength}};
}

auto RingBuffer::writableSpans() noexcept -> std::array<ByteSpan, 2> {
    const auto freeLength = _storage.size() - _length;
    if (freeLength == 0U) {
        return {};
    }
    const auto writePosition = (_readPosition + _length) % _storage.size();
    const auto firstLength = std::min(freeLength, _storage.size() - writePosition);
    const auto secondLength = freeLength - firstLength;
    return {ByteSpan{_storage.data() + writePosition, firstLength}, ByteSpan{_storage.data(), secondLength}};
}

void RingBuffer::commitWritten(const std::size_t length) {
    if (length > _storage.size() - _length) {
        throw err::ParameterError{"Committed byte count exceeds ring buffer space.", "length"};
    }
    _length += length;
}

void RingBuffer::consumeRead(const std::size_t length) {
    if (length > _length) {
        throw err::ParameterError{"Consumed byte count exceeds readable ring buffer data.", "length"};
    }
    if (_sensitive) {
        eraseReadablePrefix(length);
    }
    if (length == _length) {
        _readPosition = 0U;
        _length = 0U;
        return;
    }
    _readPosition = (_readPosition + length) % _storage.size();
    _length -= length;
}

void RingBuffer::eraseReadablePrefix(const std::size_t length) noexcept {
    auto remaining = length;
    for (const auto span : readableSpans()) {
        const auto count = std::min(span.size(), remaining);
        if (count > 0U) {
            mem::secureErase(ByteSpan{const_cast<Byte *>(span.data()), count});
            remaining -= count;
        }
    }
}

void RingBuffer::beginUnsafeAccess() {
    if (_unsafeAccessActive) {
        throw err::LogicError{"Ring buffer already has an active unsafe access lease."};
    }
    _unsafeAccessActive = true;
}

void RingBuffer::endUnsafeAccess() noexcept {
    _unsafeAccessActive = false;
}

void RingBuffer::verifySafeAccess() const {
    if (_unsafeAccessActive) {
        throw err::LogicError{"Safe ring buffer access is not allowed while an unsafe access lease is active."};
    }
}

}
