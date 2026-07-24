// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardTextOutputStream.hpp"

#include "IoService.hpp"
#include "StreamBufferSizes.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../time/TimePoint.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <iterator>
#include <mutex>
#include <utility>

namespace erbsland::stream::impl {

using namespace text::literals;
using text::Char;
using text::String;
using text::StringEncoding;

class StandardTextOutputStream::Data final : public std::enable_shared_from_this<Data> {
public:
    explicit Data(NativeOutputStreamPtr output) : native{std::move(output)} {}

    void schedule() {
        if (workInProgress || aborted.load()) {
            return;
        }
        if (queue.empty()) {
            if (streamState.load() == StreamState::Closing) {
                scheduleFlush(true, 0U);
            }
            return;
        }
        workInProgress = true;
        auto self = shared_from_this();
        auto text = queue.front();
        IoService::submitIoWork(
            [self = std::move(self), text = std::move(text)]() -> void { self->performWrite(text); });
    }

    void performWrite(const String &text) {
        auto failure = std::exception_ptr{};
        try {
            native->writeText(text);
        } catch (...) {
            failure = std::current_exception();
        }
        {
            const auto lock = std::scoped_lock{mutex};
            workInProgress = false;
            if (failure) {
                error = failure;
                streamState.store(StreamState::Failed);
            } else if (!aborted.load()) {
                pendingBytes -= text.length().toSizeT();
                queue.pop_front();
                schedule();
            }
        }
        condition.notify_all();
    }

    void scheduleFlush(const bool closeAfterFlush, const uint64_t generation) {
        if (workInProgress || aborted.load()) {
            return;
        }
        workInProgress = true;
        auto self = shared_from_this();
        IoService::submitIoWork([self = std::move(self), closeAfterFlush, generation]() -> void {
            self->performFlush(closeAfterFlush, generation);
        });
    }

    void performFlush(const bool closeAfterFlush, const uint64_t generation) {
        auto failure = std::exception_ptr{};
        try {
            native->flush();
        } catch (...) {
            failure = std::current_exception();
        }
        {
            const auto lock = std::scoped_lock{mutex};
            workInProgress = false;
            if (failure) {
                error = failure;
                streamState.store(StreamState::Failed);
            } else if (closeAfterFlush) {
                streamState.store(StreamState::Closed);
            } else {
                completedFlushGeneration = generation;
                schedule();
            }
        }
        condition.notify_all();
    }

public:
    NativeOutputStreamPtr native;
    OutputStreamSettings settings;
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::deque<String> queue;
    std::size_t pendingBytes{0U};
    std::atomic<StreamState> streamState{StreamState::Open};
    std::atomic<bool> aborted{false};
    bool workInProgress{false};
    uint64_t flushGeneration{0U};
    uint64_t completedFlushGeneration{0U};
    std::exception_ptr error;
};

StandardTextOutputStream::StandardTextOutputStream(NativeOutputStreamPtr nativeOutputStream) :
    _data{std::make_shared<Data>(std::move(nativeOutputStream))} {
    if (!_data->native) {
        throw StreamError{StreamErrorContext{
            "Failed to create the standard text output stream."_el,
            "The required native output stream was not provided."_el}};
    }
}

auto StandardTextOutputStream::createErrorContext() const noexcept -> StreamErrorContext {
    return _data->native->createErrorContext();
}

auto StandardTextOutputStream::encoding() const noexcept -> StringEncoding {
    return StringEncoding::Utf8;
}

auto StandardTextOutputStream::effectiveEncoding() const noexcept -> StringEncoding {
    return StringEncoding::Utf8;
}

auto StandardTextOutputStream::outputSettings() const noexcept -> const OutputStreamSettings & {
    return _data->settings;
}

auto StandardTextOutputStream::state() const noexcept -> StreamState {
    return _data->streamState.load();
}

auto StandardTextOutputStream::isReady() const noexcept -> bool {
    const auto lock = std::scoped_lock{_data->mutex};
    return _data->pendingBytes <= streamBufferSizes(_data->settings.buffering()).ioRing.toSizeT() &&
        state() == StreamState::Open;
}

auto StandardTextOutputStream::waitForReady() -> StreamWaitStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto ready = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this]() -> bool {
        return _data->pendingBytes <= streamBufferSizes(_data->settings.buffering()).ioRing.toSizeT() || _data->error ||
            state() != StreamState::Open;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return ready && state() == StreamState::Open ? StreamWaitStatus::Ready : StreamWaitStatus::Timeout;
}

auto StandardTextOutputStream::flush() -> StreamWriteStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    const auto drained = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this]() -> bool {
        return (_data->queue.empty() && !_data->workInProgress) || _data->error || state() != StreamState::Open;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    if (!drained || state() != StreamState::Open) {
        return StreamWriteStatus::Timeout;
    }
    const auto generation = ++_data->flushGeneration;
    _data->scheduleFlush(false, generation);
    const auto flushed = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this, generation]() -> bool {
        return _data->completedFlushGeneration >= generation || _data->error;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return flushed ? StreamWriteStatus::Success : StreamWriteStatus::Timeout;
}

auto StandardTextOutputStream::close() -> StreamCloseStatus {
    auto lock = std::unique_lock{_data->mutex};
    if (state() == StreamState::Closed) {
        return StreamCloseStatus::Closed;
    }
    if (state() == StreamState::Open) {
        _data->streamState.store(StreamState::Closing);
        _data->schedule();
    }
    const auto closed = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this]() -> bool {
        return state() == StreamState::Closed || state() == StreamState::Failed;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return closed ? StreamCloseStatus::Closed : StreamCloseStatus::Timeout;
}

void StandardTextOutputStream::abort() noexcept {
    const auto wasAborted = _data->aborted.exchange(true);
    _data->streamState.store(StreamState::Closed);
    if (!wasAborted) {
        _data->native->abort();
    }
    if (_data->mutex.try_lock()) {
        if (_data->workInProgress && !_data->queue.empty()) {
            const auto frontLength = _data->queue.front().length().toSizeT();
            _data->queue.erase(std::next(_data->queue.begin()), _data->queue.end());
            _data->pendingBytes = frontLength;
        } else {
            _data->queue.clear();
            _data->pendingBytes = 0U;
        }
        _data->mutex.unlock();
    }
    _data->condition.notify_all();
}

auto StandardTextOutputStream::write(const Char character) -> StreamWriteStatus {
    return write(String::fromCharacter(character));
}

auto StandardTextOutputStream::write(const String &text) -> StreamWriteStatus {
    const auto byteLength = text.length().toSizeT();
    const auto requestLimit = _data->settings.backBufferLimit().toSizeTOrThrow();
    if (byteLength > requestLimit) {
        throwError(
            "Failed to write text to the output stream."_el,
            "The complete text request exceeds the configured output buffer limit."_el);
    }
    const auto capacity = streamBufferSizes(_data->settings.buffering()).ioRing.toSizeTOrThrow() + requestLimit;
    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    while (_data->pendingBytes + byteLength > capacity) {
        if (_data->error) {
            std::rethrow_exception(_data->error);
        }
        if (_data->condition.wait_until(lock, deadline.toStdTimePoint()) == std::cv_status::timeout) {
            return StreamWriteStatus::Timeout;
        }
    }
    if (state() != StreamState::Open) {
        throwError("Failed to write text to the output stream."_el, "The standard output stream is not open."_el);
    }
    _data->queue.emplace_back(text);
    _data->pendingBytes += byteLength;
    _data->schedule();
    return StreamWriteStatus::Success;
}

auto StandardTextOutputStream::writeLine() -> StreamWriteStatus {
    return write(String::fromCharacter(U'\n'));
}

auto StandardTextOutputStream::writeLine(const String &text) -> StreamWriteStatus {
    return write(String::fromJoined({text, "\n"_el}));
}

}
