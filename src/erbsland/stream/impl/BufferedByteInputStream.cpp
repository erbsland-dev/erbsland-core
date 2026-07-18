// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferedByteInputStream.hpp"

#include "BufferedByteInputStreamData.hpp"

#include "../../text/Literals.hpp"

#include <exception>
#include <mutex>
#include <utility>

namespace erbsland::stream::impl {

using namespace text::literals;

using unit::ByteIndex;
using unit::ByteLength;
using unit::ByteOffset;

BufferedByteInputStream::BufferedByteInputStream(NativeByteStreamPtr nativeStream, InputStreamSettings settings) :
    _data{std::make_shared<BufferedByteInputStreamData>(std::move(nativeStream), settings)} {
    const auto lock = std::scoped_lock{_data->mutex};
    _data->scheduleRead();
}

BufferedByteInputStream::~BufferedByteInputStream() {
    abort();
}

auto BufferedByteInputStream::inputSettings() const noexcept -> const InputStreamSettings & {
    return _data->settings;
}

auto BufferedByteInputStream::state() const noexcept -> StreamState {
    return _data->streamState.load();
}

auto BufferedByteInputStream::isReady() const noexcept -> bool {
    if (hasRetainedInput()) {
        return true;
    }
    const auto lock = std::scoped_lock{_data->mutex};
    return !_data->front.isEmpty() || _data->finished || _data->error || state() != StreamState::Open;
}

auto BufferedByteInputStream::waitForReady() -> StreamWaitStatus {
    if (hasRetainedInput()) {
        return StreamWaitStatus::Ready;
    }
    auto lock = std::unique_lock{_data->mutex};
    const auto ready = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this]() -> bool {
        return !_data->front.isEmpty() || _data->finished || _data->error || state() != StreamState::Open;
    });
    return ready ? StreamWaitStatus::Ready : StreamWaitStatus::Timeout;
}

auto BufferedByteInputStream::close() -> StreamCloseStatus {
    abort();
    return StreamCloseStatus::Closed;
}

void BufferedByteInputStream::abort() noexcept {
    discardRetainedInput();
    const auto wasAborted = _data->aborted.exchange(true);
    _data->streamState.store(StreamState::Closed);
    if (!wasAborted) {
        _data->native->abort();
    }
    if (_data->mutex.try_lock()) {
        if (!_data->readInProgress) {
            _data->back.clear();
        }
        _data->front.clear();
        _data->mutex.unlock();
    }
    _data->condition.notify_all();
}

auto BufferedByteInputStream::createErrorContext() const noexcept -> StreamErrorContext {
    return _data->native->createErrorContext();
}

auto BufferedByteInputStream::readFromSource(const std::span<mem::Byte> destination, const ReadDeadline deadline)
    -> StreamReadResult<ByteLength> {
    if (destination.empty()) {
        return {StreamReadStatus::Data, ByteLength::zero()};
    }
    auto lock = std::unique_lock{_data->mutex};
    const auto ready = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this]() -> bool {
        return !_data->front.isEmpty() || _data->finished || _data->error || state() != StreamState::Open;
    });
    if (!ready) {
        return {StreamReadStatus::Timeout, ByteLength::zero()};
    }
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    if (_data->front.isEmpty()) {
        return {StreamReadStatus::Finished, ByteLength::zero()};
    }
    const auto readLength = _data->front.read(destination);
    _data->logicalPosition.fetch_add(readLength.toRawValue());
    if (_data->front.isEmpty() && !_data->back.isEmpty()) {
        _data->front.swap(_data->back);
    }
    _data->scheduleRead();
    return {StreamReadStatus::Data, readLength};
}

auto BufferedByteInputStream::sourceSupportsPositioning() const noexcept -> bool {
    return _data->native->supportsPositioning();
}

auto BufferedByteInputStream::sourcePosition() const -> ByteIndex {
    if (!sourceSupportsPositioning()) {
        return StreamPositioning::position();
    }
    return ByteIndex{_data->logicalPosition.load()};
}

auto BufferedByteInputStream::setSourcePosition(const ByteIndex position) -> StreamPositionStatus {
    if (!sourceSupportsPositioning()) {
        return StreamPositioning::setPosition(position);
    }
    auto lock = std::unique_lock{_data->mutex};
    if (!beginPositioning(lock, deadlineFromNow())) {
        return StreamPositionStatus::Timeout;
    }
    try {
        completePositioning(_data->native->setPosition(position));
    } catch (...) {
        cancelPositioning();
        throw;
    }
    return StreamPositionStatus::Success;
}

auto BufferedByteInputStream::moveSourcePosition(const StreamPositionOrigin origin, const ByteOffset offset)
    -> StreamPositionStatus {
    if (!sourceSupportsPositioning()) {
        return StreamPositioning::movePosition(origin, offset);
    }
    auto lock = std::unique_lock{_data->mutex};
    if (!beginPositioning(lock, deadlineFromNow())) {
        return StreamPositionStatus::Timeout;
    }
    try {
        auto result = ByteIndex{};
        if (origin == StreamPositionOrigin::Current) {
            const auto current = ByteIndex{_data->logicalPosition.load()};
            result = _data->native->setPosition(current.movedOrThrow(offset));
        } else {
            result = _data->native->movePosition(origin, offset);
        }
        completePositioning(result);
    } catch (...) {
        cancelPositioning();
        throw;
    }
    return StreamPositionStatus::Success;
}

auto BufferedByteInputStream::beginPositioning(std::unique_lock<std::mutex> &lock, const ReadDeadline deadline)
    -> bool {
    _data->positioning = true;
    const auto ready = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this]() -> bool {
        return !_data->readInProgress || _data->error || state() != StreamState::Open;
    });
    if (_data->error) {
        cancelPositioning();
        std::rethrow_exception(_data->error);
    }
    if (state() != StreamState::Open) {
        cancelPositioning();
        throwError("Failed to position the input stream."_el, "The input stream is not open."_el);
    }
    if (!ready) {
        cancelPositioning();
        return false;
    }
    return true;
}

void BufferedByteInputStream::completePositioning(const ByteIndex position) {
    _data->front.clear();
    _data->back.clear();
    _data->finished = false;
    _data->logicalPosition.store(position.toRawValue());
    _data->positioning = false;
    _data->scheduleRead();
    _data->condition.notify_all();
}

void BufferedByteInputStream::cancelPositioning() noexcept {
    _data->positioning = false;
    _data->scheduleRead();
    _data->condition.notify_all();
}

}
