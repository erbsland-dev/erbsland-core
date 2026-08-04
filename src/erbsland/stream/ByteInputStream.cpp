// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteInputStream.hpp"

#include "impl/StreamBufferSizes.hpp"

#include "../err/ParameterError.hpp"
#include "../mem/impl/UnsafeRingBufferAccess.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace erbsland::stream {

using namespace text::literals;
using mem::Byte;
using mem::ByteBlock;
using mem::Endianness;
using unit::ByteIndex;
using unit::ByteLength;
using unit::ByteOffset;

auto ByteInputStream::endianness() const noexcept -> Endianness {
    return _endianness;
}

void ByteInputStream::setEndianness(const Endianness endianness) noexcept {
    _endianness = endianness;
}

auto ByteInputStream::supportsPositioning() const noexcept -> bool {
    return sourceSupportsPositioning();
}

auto ByteInputStream::position() const -> ByteIndex {
    return sourcePosition();
}

auto ByteInputStream::setPosition(const ByteIndex position) -> StreamPositionStatus {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamPositionStatus::Timeout;
    }
    const auto result = setSourcePosition(position);
    if (result.isSuccess()) {
        clearRetainedInput();
    }
    return result;
}

auto ByteInputStream::movePosition(const StreamPositionOrigin origin, const ByteOffset offset) -> StreamPositionStatus {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamPositionStatus::Timeout;
    }
    const auto result = moveSourcePosition(origin, offset);
    if (result.isSuccess()) {
        clearRetainedInput();
    }
    return result;
}

auto ByteInputStream::deadlineFromNow() const -> ReadDeadline {
    return time::TimePoint::inFuture(inputSettings().timeout());
}

auto ByteInputStream::retainedBuffer() -> mem::RingBuffer & {
    if (!_retainedBytes) {
        const auto initialCapacity = impl::streamBufferSizes(inputSettings().buffering()).ioRing;
        const auto maximumCapacity = ByteLength::fromSizeT(std::numeric_limits<std::size_t>::max());
        _retainedBytes.emplace(initialCapacity, maximumCapacity);
        _retainedBytes->setSensitive(inputSettings().isSensitive());
    } else if (_retainedBytes->isSensitive() != inputSettings().isSensitive()) {
        _retainedBytes->secureErase();
        _retainedBytes->setSensitive(inputSettings().isSensitive());
    }
    return *_retainedBytes;
}

auto ByteInputStream::readChunkLocked(const mem::ByteSpan destination, const ReadDeadline deadline)
    -> StreamReadResult<ByteLength> {
    if (destination.empty()) {
        return {StreamReadStatus::Data, ByteLength::zero()};
    }
    auto &retained = retainedBuffer();
    if (!retained.isEmpty()) {
        return {StreamReadStatus::Data, retained.read(destination)};
    }
    return readFromSource(destination, deadline);
}

auto ByteInputStream::readIntoRetained(const ByteLength maximumLength, const ReadDeadline deadline)
    -> StreamReadStatus {
    if (maximumLength.isZero()) {
        return StreamReadStatus::Data;
    }
    auto &retained = retainedBuffer();
    if (isFailure(retained.reserveAdditional(maximumLength))) {
        throwError(
            "Failed to retain byte-stream data."_el,
            "The requested aggregate byte length exceeds the supported storage size."_el);
    }
    auto access = mem::impl::UnsafeRingBufferAccess{retained};
    const auto spans = access.writableSpans();
    auto destination = spans[0];
    if (destination.empty()) {
        destination = spans[1];
    }
    destination = destination.first(std::min(destination.size(), maximumLength.toSizeT()));
    const auto result = readFromSource(destination, deadline);
    if (result != StreamReadStatus::Data) {
        return result.status();
    }
    if (result.data().isZero() || result.data().toSizeT() > destination.size()) {
        throwError(
            "Failed to read byte-stream data."_el, "The byte input source reported an invalid number of bytes."_el);
    }
    access.commitWritten(result.data());
    return StreamReadStatus::Data;
}

auto ByteInputStream::prepareRead(const ByteLength maximumLength, const ReadDeadline deadline) -> StreamReadStatus {
    if (maximumLength.isZero() || !retainedBuffer().isEmpty()) {
        return StreamReadStatus::Data;
    }
    return readIntoRetained(maximumLength, deadline);
}

auto ByteInputStream::prepareExact(const ByteLength length, const ReadDeadline deadline) -> StreamReadStatus {
    auto &retained = retainedBuffer();
    while (retained.length() < length) {
        const auto status = readIntoRetained(length - retained.length(), deadline);
        if (status != StreamReadStatus::Data) {
            return status;
        }
    }
    return StreamReadStatus::Data;
}

auto ByteInputStream::prepareAll(const ByteLength maximumLength, const ReadDeadline deadline) -> StreamReadStatus {
    auto &retained = retainedBuffer();
    while (retained.length() < maximumLength) {
        const auto maximumRead = std::min(
            impl::streamBufferSizes(inputSettings().buffering()).aggregateChunk, maximumLength - retained.length());
        const auto status = readIntoRetained(maximumRead, deadline);
        if (status == StreamReadStatus::Timeout) {
            return status;
        }
        if (status == StreamReadStatus::Finished) {
            return retained.isEmpty() ? StreamReadStatus::Finished : StreamReadStatus::Data;
        }
    }
    return StreamReadStatus::Data;
}

auto ByteInputStream::takeRetained(const ByteLength length) -> ByteBlock {
    return retainedBuffer().read(length);
}

auto ByteInputStream::read(const mem::ByteSpan destination) -> StreamReadResult<ByteLength> {
    return readUntil(destination, deadlineFromNow());
}

auto ByteInputStream::readUntil(const mem::ByteSpan destination, const ReadDeadline deadline)
    -> StreamReadResult<ByteLength> {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, ByteLength::zero()};
    }
    return readChunkLocked(destination, deadline);
}

auto ByteInputStream::read(const ByteLength maximumLength) -> StreamReadResult<ByteBlock> {
    if (maximumLength.isInfinite()) {
        throw err::ParameterError{"The maximum byte read length must be finite."_el, "maximumLength"_el};
    }
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, {}};
    }
    const auto status = prepareRead(maximumLength, deadlineFromNow());
    return status == StreamReadStatus::Data ? StreamReadResult<ByteBlock>{status, takeRetained(maximumLength)}
                                            : StreamReadResult<ByteBlock>{status, {}};
}

auto ByteInputStream::readExact(const ByteLength length) -> StreamReadResult<ByteBlock> {
    if (length.isInfinite()) {
        throw err::ParameterError{"The exact byte read length must be finite."_el, "length"_el};
    }
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, {}};
    }
    const auto status = prepareExact(length, deadlineFromNow());
    return status == StreamReadStatus::Data ? StreamReadResult<ByteBlock>{status, takeRetained(length)}
                                            : StreamReadResult<ByteBlock>{status, {}};
}

auto ByteInputStream::readByte() -> StreamReadResult<Byte> {
    auto byte = Byte{};
    const auto result = read(mem::ByteSpan{&byte, 1U});
    return result == StreamReadStatus::Data ? StreamReadResult<Byte>{StreamReadStatus::Data, byte}
                                            : StreamReadResult<Byte>{result.status(), {}};
}

auto ByteInputStream::readAll() -> StreamReadResult<ByteBlock> {
    return readAll(cDefaultByteReadMaximum);
}

auto ByteInputStream::readAll(const ByteLength maximumLength) -> StreamReadResult<ByteBlock> {
    if (maximumLength.isInfinite()) {
        throw err::ParameterError{"The maximum aggregate byte length must be finite."_el, "maximumLength"_el};
    }
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, {}};
    }
    const auto status = prepareAll(maximumLength, deadlineFromNow());
    return status == StreamReadStatus::Data ? StreamReadResult<ByteBlock>{status, takeRetained(maximumLength)}
                                            : StreamReadResult<ByteBlock>{status, {}};
}

auto ByteInputStream::coRead(const ByteLength maximumLength) -> util::CoTask<StreamReadResult<ByteBlock>> {
    auto self = std::static_pointer_cast<ByteInputStream>(sharedInputStream());
    return util::CoTask<StreamReadResult<ByteBlock>>::run(
        [self = std::move(self), maximumLength]() -> StreamReadResult<ByteBlock> { return self->read(maximumLength); });
}

auto ByteInputStream::coReadExact(const ByteLength length) -> util::CoTask<StreamReadResult<ByteBlock>> {
    auto self = std::static_pointer_cast<ByteInputStream>(sharedInputStream());
    return util::CoTask<StreamReadResult<ByteBlock>>::run(
        [self = std::move(self), length]() -> StreamReadResult<ByteBlock> { return self->readExact(length); });
}

auto ByteInputStream::coReadAll() -> util::CoTask<StreamReadResult<ByteBlock>> {
    return coReadAll(cDefaultByteReadMaximum);
}

auto ByteInputStream::coReadAll(const ByteLength maximumLength) -> util::CoTask<StreamReadResult<ByteBlock>> {
    auto self = std::static_pointer_cast<ByteInputStream>(sharedInputStream());
    return util::CoTask<StreamReadResult<ByteBlock>>::run(
        [self = std::move(self), maximumLength]() -> StreamReadResult<ByteBlock> {
            return self->readAll(maximumLength);
        });
}

auto ByteInputStream::coReadBlocks() -> util::CoAsyncGenerator<StreamReadResult<ByteBlock>> {
    return coReadBlocks(cDefaultByteReadMaximum);
}

auto ByteInputStream::coReadBlocks(const ByteLength maximumLength)
    -> util::CoAsyncGenerator<StreamReadResult<ByteBlock>> {
    if (maximumLength.isInfinite() || maximumLength.isZero()) {
        throw err::ParameterError{
            "The coroutine byte-block length must be positive and finite."_el, "maximumLength"_el};
    }
    while (true) {
        auto result = co_await coRead(maximumLength);
        if (result == StreamReadStatus::Finished) {
            co_return;
        }
        co_yield std::move(result);
    }
}

auto ByteInputStream::hasRetainedInput() const noexcept -> bool {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    return lock.owns_lock() && _retainedBytes && !_retainedBytes->isEmpty();
}

void ByteInputStream::discardRetainedInput() noexcept {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (lock.owns_lock()) {
        clearRetainedInput();
    }
}

auto ByteInputStream::sourceSupportsPositioning() const noexcept -> bool {
    return false;
}

auto ByteInputStream::sourcePosition() const -> ByteIndex {
    return StreamPositioning::position();
}

auto ByteInputStream::setSourcePosition(const ByteIndex position) -> StreamPositionStatus {
    return StreamPositioning::setPosition(position);
}

auto ByteInputStream::moveSourcePosition(const StreamPositionOrigin origin, const ByteOffset offset)
    -> StreamPositionStatus {
    return StreamPositioning::movePosition(origin, offset);
}

void ByteInputStream::clearRetainedInput() noexcept {
    if (_retainedBytes) {
        _retainedBytes->clear();
    }
}

}
