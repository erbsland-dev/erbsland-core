// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RingBuffer.hpp"

#include "impl/BestGrowth.hpp"
#include "impl/RingBufferStorageTraits.hpp"

#include "../err/LogicError.hpp"
#include "../err/ParameterError.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <utility>

namespace erbsland::mem {

RingBuffer::RingBuffer(const unit::ByteLength capacity) : RingBuffer{capacity, capacity} {
}

RingBuffer::RingBuffer(const unit::ByteLength initialCapacity, const unit::ByteLength maximumCapacity) :
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

auto RingBuffer::capacity() const noexcept -> unit::ByteLength {
    return unit::ByteLength::fromSizeT(_storage.size());
}

auto RingBuffer::maximumCapacity() const noexcept -> unit::ByteLength {
    return unit::ByteLength::fromSizeT(_maximumCapacity);
}

auto RingBuffer::length() const noexcept -> unit::ByteLength {
    return unit::ByteLength::fromSizeT(_length);
}

auto RingBuffer::available() const noexcept -> unit::ByteLength {
    return unit::ByteLength::fromSizeT(_storage.size() - _length);
}

auto RingBuffer::canWrite(const unit::ByteLength length) const noexcept -> bool {
    return !length.isInfinite() && length.toSizeT() <= _maximumCapacity - _length;
}

auto RingBuffer::reserveAdditional(const unit::ByteLength length) -> util::Result {
    verifySafeAccess();
    if (!canWrite(length)) {
        return util::Result::Failure;
    }
    const auto required = _length + length.toSizeT();
    if (required <= _storage.size()) {
        return util::Result::Success;
    }

    const auto bestCapacity = impl::bestGrowthCapacity<impl::RingBufferStorageTraits>(_storage.size(), required);
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
    _storage = std::move(newStorage);
    _readPosition = 0U;
    return util::Result::Success;
}

auto RingBuffer::write(const std::span<const Byte> bytes) -> unit::ByteLength {
    verifySafeAccess();
    const auto maximumWrite = std::min(bytes.size(), _maximumCapacity - _length);
    if (isFailure(reserveAdditional(unit::ByteLength::fromSizeT(maximumWrite)))) {
        return unit::ByteLength::zero();
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
    return unit::ByteLength::fromSizeT(maximumWrite);
}

auto RingBuffer::writeExact(const std::span<const Byte> bytes) -> util::Result {
    verifySafeAccess();
    if (isFailure(reserveAdditional(unit::ByteLength::fromSizeT(bytes.size())))) {
        return util::Result::Failure;
    }
    return write(bytes) == unit::ByteLength::fromSizeT(bytes.size()) ? util::Result::Success : util::Result::Failure;
}

auto RingBuffer::read(const std::span<Byte> destination) -> unit::ByteLength {
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
    return unit::ByteLength::fromSizeT(maximumRead);
}

auto RingBuffer::read(const unit::ByteLength maximum) -> ByteBlock {
    verifySafeAccess();
    const auto readLength = maximum.isInfinite() ? _length : std::min(maximum.toSizeT(), _length);
    auto bytes = std::vector<Byte>(readLength);
    static_cast<void>(read(std::span<Byte>{bytes}));
    return ByteBlock{bytes};
}

void RingBuffer::clear() noexcept {
    if (_unsafeAccessActive) {
        std::terminate();
    }
    _readPosition = 0U;
    _length = 0U;
}

void RingBuffer::shrinkToInitial() {
    verifySafeAccess();
    if (!isEmpty() || _storage.size() == _initialCapacity) {
        return;
    }
    _storage = std::vector<Byte>(_initialCapacity);
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
}

auto RingBuffer::readableSpans() const noexcept -> std::array<std::span<const Byte>, 2> {
    if (_length == 0U) {
        return {};
    }
    const auto firstLength = std::min(_length, _storage.size() - _readPosition);
    const auto secondLength = _length - firstLength;
    return {
        std::span<const Byte>{_storage.data() + _readPosition, firstLength},
        std::span<const Byte>{_storage.data(), secondLength}};
}

auto RingBuffer::writableSpans() noexcept -> std::array<std::span<Byte>, 2> {
    const auto freeLength = _storage.size() - _length;
    if (freeLength == 0U) {
        return {};
    }
    const auto writePosition = (_readPosition + _length) % _storage.size();
    const auto firstLength = std::min(freeLength, _storage.size() - writePosition);
    const auto secondLength = freeLength - firstLength;
    return {
        std::span<Byte>{_storage.data() + writePosition, firstLength}, std::span<Byte>{_storage.data(), secondLength}};
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
    if (length == _length) {
        _readPosition = 0U;
        _length = 0U;
        return;
    }
    _readPosition = (_readPosition + length) % _storage.size();
    _length -= length;
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
