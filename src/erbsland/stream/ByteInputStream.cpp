// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteInputStream.hpp"

#include "../err/ParameterError.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <cstring>

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

auto ByteInputStream::readChunkLocked(const std::span<Byte> destination, const ReadDeadline deadline)
    -> StreamReadResult<ByteLength> {
    if (destination.empty()) {
        return {StreamReadStatus::Data, ByteLength::zero()};
    }
    if (_replayPosition < _replayBytes.size()) {
        const auto count = std::min(destination.size(), _replayBytes.size() - _replayPosition);
        std::memcpy(destination.data(), _replayBytes.data() + _replayPosition, count * sizeof(Byte));
        _replayPosition += count;
        if (_replayPosition == _replayBytes.size()) {
            _replayBytes.clear();
            _replayPosition = 0U;
        }
        return {StreamReadStatus::Data, ByteLength::fromSizeT(count)};
    }
    return readFromSource(destination, deadline);
}

auto ByteInputStream::read(const std::span<Byte> destination) -> StreamReadResult<ByteLength> {
    return readUntil(destination, deadlineFromNow());
}

auto ByteInputStream::readUntil(const std::span<Byte> destination, const ReadDeadline deadline)
    -> StreamReadResult<ByteLength> {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, ByteLength::zero()};
    }
    cancelAggregateRead();
    return readChunkLocked(destination, deadline);
}

auto ByteInputStream::read(const ByteLength maximumLength) -> StreamReadResult<ByteBlock> {
    if (maximumLength.isInfinite()) {
        throw err::ParameterError{"The maximum byte read length must be finite.", "maximumLength"};
    }
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, ByteBlock{}};
    }
    cancelAggregateRead();
    if (maximumLength.isZero()) {
        return {StreamReadStatus::Data, ByteBlock{}};
    }
    auto buffer = mem::impl::UnsafeByteBlockBuffer{maximumLength};
    const auto result = readChunkLocked(buffer.data(), deadlineFromNow());
    if (result != StreamReadStatus::Data) {
        return {result, ByteBlock{}};
    }
    return {StreamReadStatus::Data, buffer.take(result.data())};
}

auto ByteInputStream::readExact(const ByteLength length) -> StreamReadResult<ByteBlock> {
    if (length.isInfinite()) {
        throw err::ParameterError{"The exact byte read length must be finite.", "length"};
    }
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, ByteBlock{}};
    }
    selectAggregateRead(AggregateReadKind::Exact, length);
    if (length.isZero()) {
        return {StreamReadStatus::Data, takePending()};
    }
    const auto deadline = deadlineFromNow();
    auto buffer = std::array<Byte, 16U * 1024U>{};
    while (_pendingBytes.size() < length.toSizeT()) {
        const auto remaining = length.toSizeT() - _pendingBytes.size();
        const auto result =
            readChunkLocked(std::span<Byte>{buffer.data(), std::min(buffer.size(), remaining)}, deadline);
        if (result != StreamReadStatus::Data) {
            return {result.status(), ByteBlock{}};
        }
        if (result.data().isZero()) {
            throwError(
                "Failed to read byte-stream data."_el,
                "The byte input source reported data without providing any bytes."_el);
        }
        appendPending(std::span<const Byte>{buffer.data(), result.data().toSizeT()});
    }
    return {StreamReadStatus::Data, takePending()};
}

auto ByteInputStream::readByte() -> StreamReadResult<Byte> {
    auto byte = Byte{};
    const auto result = read(std::span<Byte>{&byte, 1U});
    if (result != StreamReadStatus::Data) {
        return {result, Byte{}};
    }
    return {StreamReadStatus::Data, byte};
}

auto ByteInputStream::readAll() -> StreamReadResult<ByteBlock> {
    return readAll(cDefaultByteReadMaximum);
}

auto ByteInputStream::readAll(const ByteLength maximumLength) -> StreamReadResult<ByteBlock> {
    constexpr auto cBufferSize = std::size_t{16U * 1024U};

    if (maximumLength.isInfinite()) {
        throw err::ParameterError{"The maximum aggregate byte length must be finite.", "maximumLength"};
    }
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, ByteBlock{}};
    }
    selectAggregateRead(AggregateReadKind::All, maximumLength);
    if (maximumLength.isZero()) {
        return {StreamReadStatus::Data, takePending()};
    }
    const auto deadline = deadlineFromNow();
    auto buffer = std::array<Byte, cBufferSize>{};
    while (_pendingBytes.size() < maximumLength.toSizeT()) {
        const auto remaining = std::min(cBufferSize, maximumLength.toSizeT() - _pendingBytes.size());
        const auto readResult = readChunkLocked(std::span<Byte>{buffer.data(), remaining}, deadline);
        if (readResult == StreamReadStatus::Timeout) {
            return {StreamReadStatus::Timeout, ByteBlock{}};
        }
        if (readResult == StreamReadStatus::Finished) {
            if (_pendingBytes.empty()) {
                _aggregateKind = AggregateReadKind::None;
                _aggregateTarget = {};
                return {StreamReadStatus::Finished, ByteBlock{}};
            }
            return {StreamReadStatus::Data, takePending()};
        }
        if (readResult.data().isZero()) {
            throwError(
                "Failed to read byte-stream data."_el,
                "The byte input source reported data without providing any bytes."_el);
        }
        appendPending(std::span<const Byte>{buffer.data(), readResult.data().toSizeT()});
    }
    return {StreamReadStatus::Data, takePending()};
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
        throw err::ParameterError{"The coroutine byte-block length must be positive and finite.", "maximumLength"};
    }
    while (true) {
        auto result = co_await coRead(maximumLength);
        if (result == StreamReadStatus::Finished) {
            co_return;
        }
        co_yield std::move(result);
    }
}

void ByteInputStream::selectAggregateRead(const AggregateReadKind kind, const ByteLength target) {
    if (_aggregateKind == kind && _aggregateTarget == target) {
        return;
    }
    cancelAggregateRead();
    _aggregateKind = kind;
    _aggregateTarget = target;
}

void ByteInputStream::cancelAggregateRead() {
    if (_aggregateKind == AggregateReadKind::None) {
        return;
    }
    if (!_pendingBytes.empty()) {
        auto replay = std::vector<Byte>{};
        replay.reserve(_pendingBytes.size() + _replayBytes.size() - _replayPosition);
        replay.insert(replay.end(), _pendingBytes.begin(), _pendingBytes.end());
        replay.insert(
            replay.end(), _replayBytes.begin() + static_cast<std::ptrdiff_t>(_replayPosition), _replayBytes.end());
        _replayBytes = std::move(replay);
        _replayPosition = 0U;
    }
    _pendingBytes.clear();
    _aggregateKind = AggregateReadKind::None;
    _aggregateTarget = {};
}

void ByteInputStream::appendPending(const std::span<const Byte> bytes) {
    _pendingBytes.insert(_pendingBytes.end(), bytes.begin(), bytes.end());
}

auto ByteInputStream::takePending() -> ByteBlock {
    auto result = ByteBlock{std::span<const Byte>{_pendingBytes}};
    _pendingBytes.clear();
    _aggregateKind = AggregateReadKind::None;
    _aggregateTarget = {};
    return result;
}

auto ByteInputStream::hasRetainedInput() const noexcept -> bool {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    return lock.owns_lock() && _aggregateKind == AggregateReadKind::None && _replayPosition < _replayBytes.size();
}

void ByteInputStream::discardRetainedInput() noexcept {
    const auto lock = std::unique_lock{_readMutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return;
    }
    clearRetainedInput();
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
    _pendingBytes.clear();
    _replayBytes.clear();
    _replayPosition = 0U;
    _aggregateKind = AggregateReadKind::None;
    _aggregateTarget = {};
}

}
