// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferedByteOutputStream.hpp"

#include "BufferedByteOutputStreamData.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../mem/impl/RingBufferWriter.hpp"
#include "../../text/impl/StringEncodingWriter.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEncoder.hpp"
#include "../../time/TimePoint.hpp"

#include <algorithm>
#include <concepts>
#include <exception>
#include <mutex>
#include <utility>

namespace erbsland::stream::impl {

using namespace mem;
using namespace text;
using namespace unit;
using namespace text::literals;

BufferedByteOutputStream::BufferedByteOutputStream(
    NativeByteStreamPtr nativeStream, OutputStreamSettings settings, util::impl::WorkerReservation admission) :
    _data{std::make_shared<BufferedByteOutputStreamData>(std::move(nativeStream), settings, std::move(admission))} {
}

BufferedByteOutputStream::~BufferedByteOutputStream() {
    abort();
}

auto BufferedByteOutputStream::fileIdentity() const noexcept -> system::FileIdentity {
    return _data->native->fileIdentity();
}

auto BufferedByteOutputStream::outputSettings() const noexcept -> const OutputStreamSettings & {
    return _data->settings;
}

auto BufferedByteOutputStream::state() const noexcept -> StreamState {
    return _data->streamState.load();
}

auto BufferedByteOutputStream::isReady() const noexcept -> bool {
    const auto lock = std::scoped_lock{_data->mutex};
    return _data->back.isEmpty() && state() == StreamState::Open;
}

auto BufferedByteOutputStream::waitForReady() -> StreamWaitStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto ready = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this]() -> bool {
        return _data->back.isEmpty() || _data->error || state() != StreamState::Open;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return ready && _data->back.isEmpty() ? StreamWaitStatus::Ready : StreamWaitStatus::Timeout;
}

auto BufferedByteOutputStream::flush() -> StreamWriteStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    const auto ready = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this]() -> bool {
        return (_data->front.isEmpty() && _data->back.isEmpty() && !_data->workInProgress) || _data->error ||
            state() != StreamState::Open;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    if (!ready || state() != StreamState::Open) {
        return StreamWriteStatus::Timeout;
    }
    _data->workInProgress = true;
    const auto generation = ++_data->flushGeneration;
    const auto data = _data;
    _data->work.submit([data, generation]() -> void {
        auto failure = std::exception_ptr{};
        try {
            data->native->flush();
        } catch (const err::RuntimeError &) {
            failure = std::current_exception();
        }
        {
            const auto taskLock = std::scoped_lock{data->mutex};
            data->workInProgress = false;
            if (failure && !data->aborted.load()) {
                data->error = failure;
                data->streamState.store(StreamState::Failed);
            } else if (!data->aborted.load()) {
                data->completedFlushGeneration = generation;
                data->scheduleWrite();
            }
        }
        data->condition.notify_all();
    });
    const auto flushed = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this, generation]() -> bool {
        return _data->completedFlushGeneration >= generation || _data->error;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return flushed ? StreamWriteStatus::Success : StreamWriteStatus::Timeout;
}

auto BufferedByteOutputStream::close() -> StreamCloseStatus {
    auto lock = std::unique_lock{_data->mutex};
    if (state() == StreamState::Closed) {
        return StreamCloseStatus::Closed;
    }
    if (state() == StreamState::Failed && _data->error) {
        std::rethrow_exception(_data->error);
    }
    if (state() == StreamState::Open) {
        _data->streamState.store(StreamState::Closing);
        _data->scheduleWrite();
    }
    const auto closed = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this]() -> bool {
        return state() == StreamState::Closed || state() == StreamState::Failed;
    });
    if (state() == StreamState::Failed && _data->error) {
        std::rethrow_exception(_data->error);
    }
    return closed ? StreamCloseStatus::Closed : StreamCloseStatus::Timeout;
}

void BufferedByteOutputStream::abort() noexcept {
    const auto wasAborted = _data->aborted.exchange(true);
    _data->streamState.store(StreamState::Closed);
    if (!wasAborted) {
        _data->native->abort();
    }
    if (_data->mutex.try_lock()) {
        if (!_data->workInProgress) {
            _data->front.clear();
        }
        _data->back.clear();
        _data->mutex.unlock();
    }
    _data->condition.notify_all();
}

auto BufferedByteOutputStream::createErrorContext() const noexcept -> StreamErrorContext {
    return _data->native->createErrorContext();
}

auto BufferedByteOutputStream::write(const ConstByteSpan bytes) -> StreamWriteStatus {
    if (bytes.empty()) {
        return StreamWriteStatus::Success;
    }
    if (bytes.size() > _data->settings.backBufferLimit().toSizeTOrThrow()) {
        throwError(
            "Failed to write to the output stream."_el,
            "The complete write request exceeds the configured output buffer limit."_el);
    }

    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    while (true) {
        if (_data->error) {
            std::rethrow_exception(_data->error);
        }
        if (state() != StreamState::Open) {
            throwError("Failed to write to the output stream."_el, "The output stream is not open."_el);
        }

        if (_data->positioning) {
            if (_data->condition.wait_until(lock, deadline.toStdTimePoint()) == std::cv_status::timeout) {
                return StreamWriteStatus::Timeout;
            }
            continue;
        }

        if (!_data->workInProgress && _data->front.isEmpty() && _data->back.isEmpty()) {
            const auto frontCount = std::min(bytes.size(), _data->front.capacity().toSizeT());
            const auto backCount = bytes.size() - frontCount;
            if (isSuccessful(_data->back.reserveAdditional(ByteCount::fromSizeT(backCount)))) {
                if (_data->front.write(ConstByteSpan{bytes.data(), frontCount}) != ByteCount::fromSizeT(frontCount)) {
                    std::terminate();
                }
                if (backCount > 0U) {
                    const auto result = _data->back.writeExact(ConstByteSpan{bytes.data() + frontCount, backCount});
                    if (isFailure(result)) {
                        std::terminate();
                    }
                }
                _data->logicalPosition.fetch_add(bytes.size());
                _data->scheduleWrite();
                return StreamWriteStatus::Success;
            }
        } else if (isSuccessful(_data->back.writeExact(bytes))) {
            _data->logicalPosition.fetch_add(bytes.size());
            _data->scheduleWrite();
            return StreamWriteStatus::Success;
        }

        if (_data->condition.wait_until(lock, deadline.toStdTimePoint()) == std::cv_status::timeout) {
            return StreamWriteStatus::Timeout;
        }
    }
}

template <typename T>
auto BufferedByteOutputStream::writeEncoded(const T &source, const StringEncoding encoding, const StringBomMode bomMode)
    -> StreamWriteStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    while (true) {
        if (_data->error) {
            std::rethrow_exception(_data->error);
        }
        if (state() != StreamState::Open) {
            throwError("Failed to write text to the output stream."_el, "The output stream is not open."_el);
        }
        if (_data->positioning) {
            if (_data->condition.wait_until(lock, deadline.toStdTimePoint()) == std::cv_status::timeout) {
                return StreamWriteStatus::Timeout;
            }
            continue;
        }
        const auto previousLength = _data->back.length();
        const auto result = [&]() -> util::Result {
            if constexpr (std::same_as<T, Char>) {
                return encodeCharacterToBack(source, encoding, bomMode);
            } else {
                return StringEncoder{source}.encodeTo(_data->back, encoding, bomMode);
            }
        }();
        if (isSuccessful(result)) {
            _data->logicalPosition.fetch_add((_data->back.length() - previousLength).toRawValue());
            _data->scheduleWrite();
            return StreamWriteStatus::Success;
        }
        if (previousLength.isZero()) {
            throwError(
                "Failed to write text to the output stream."_el,
                "The complete encoded text request exceeds the configured output buffer limit."_el);
        }
        if (_data->condition.wait_until(lock, deadline.toStdTimePoint()) == std::cv_status::timeout) {
            return StreamWriteStatus::Timeout;
        }
    }
}

auto BufferedByteOutputStream::encodeCharacterToBack(
    const Char character, const StringEncoding encoding, const StringBomMode bomMode) -> util::Result {
    auto length = encoding.bomLength(bomMode);
    length.addOrThrow(character.encodedBytes(encoding));
    if (isFailure(_data->back.reserveAdditional(length))) {
        return util::Result::Failure;
    }
    auto ringWriter = mem::impl::RingBufferWriter{_data->back};
    auto encodingWriter = text::impl::StringEncodingWriter{ringWriter, encoding};
    if (encoding.writesBom(bomMode)) {
        encodingWriter.writeBom();
    }
    encodingWriter.write(character);
    ringWriter.commit();
    return util::Result::Success;
}

auto BufferedByteOutputStream::writeEncodedText(
    const String &source, const StringEncoding encoding, const StringBomMode bomMode) -> StreamWriteStatus {
    return writeEncoded(source, encoding, bomMode);
}

auto BufferedByteOutputStream::writeEncodedCharacter(
    const Char character, const StringEncoding encoding, const StringBomMode bomMode) -> StreamWriteStatus {
    const auto normalized = character.isValidUnicode() ? character : Char::replacement();
    return writeEncoded(normalized, encoding, bomMode);
}

auto BufferedByteOutputStream::supportsPositioning() const noexcept -> bool {
    return _data->native->supportsPositioning();
}

auto BufferedByteOutputStream::position() const -> ByteIndex {
    if (!supportsPositioning()) {
        return StreamPositioning::position();
    }
    return ByteIndex::fromInteger(_data->logicalPosition.load());
}

auto BufferedByteOutputStream::setPosition(const ByteIndex position) -> StreamPositionStatus {
    if (!supportsPositioning()) {
        return StreamPositioning::setPosition(position);
    }
    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    if (!beginPositioning(lock, deadline)) {
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

auto BufferedByteOutputStream::movePosition(const StreamPositionOrigin origin, const ByteOffset offset)
    -> StreamPositionStatus {
    if (!supportsPositioning()) {
        return StreamPositioning::movePosition(origin, offset);
    }
    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    if (!beginPositioning(lock, deadline)) {
        return StreamPositionStatus::Timeout;
    }
    try {
        auto result = ByteIndex{};
        if (origin == StreamPositionOrigin::Current) {
            const auto current = ByteIndex::fromInteger(_data->logicalPosition.load());
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

auto BufferedByteOutputStream::beginPositioning(std::unique_lock<std::mutex> &lock, const time::TimePoint deadline)
    -> bool {
    _data->positioning = true;
    const auto ready = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this]() -> bool {
        return (_data->front.isEmpty() && _data->back.isEmpty() && !_data->workInProgress) || _data->error ||
            state() != StreamState::Open;
    });
    if (_data->error) {
        cancelPositioning();
        std::rethrow_exception(_data->error);
    }
    if (state() != StreamState::Open) {
        cancelPositioning();
        throwError("Failed to position the output stream."_el, "The output stream is not open."_el);
    }
    if (!ready) {
        cancelPositioning();
        return false;
    }
    return true;
}

void BufferedByteOutputStream::completePositioning(const ByteIndex position) {
    _data->logicalPosition.store(position.toRawValue());
    cancelPositioning();
}

void BufferedByteOutputStream::cancelPositioning() noexcept {
    _data->positioning = false;
    _data->condition.notify_all();
}

}
