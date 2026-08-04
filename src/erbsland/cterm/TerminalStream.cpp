// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalStream.hpp"

#include "impl/TerminalStreamData.hpp"

#include "../stream/impl/StreamBufferSizes.hpp"
#include "../text/Literals.hpp"
#include "../time/TimePoint.hpp"

#include <exception>
#include <mutex>
#include <utility>

namespace erbsland::cterm {

using namespace text::literals;
using namespace stream;
using text::Char;
using text::String;
using text::StringEncoding;

TerminalStream::TerminalStream(
    TerminalPtr terminal,
    const BlockStyle style,
    TerminalStreamSynchronizationPtr synchronization,
    const OutputStreamSettings settings) :
    _terminal{std::move(terminal)} {
    if (synchronization == nullptr) {
        synchronization = createSynchronization();
    }
    _data = std::make_shared<impl::TerminalStreamData>(_terminal, style, std::move(synchronization), settings);
}

auto TerminalStream::createSynchronization() -> TerminalStreamSynchronizationPtr {
    return std::make_shared<TerminalStreamSynchronization>();
}

auto TerminalStream::create(
    TerminalPtr terminal,
    const BlockStyle style,
    TerminalStreamSynchronizationPtr synchronization,
    const OutputStreamSettings settings) -> TerminalStreamPtr {
    return std::make_shared<TerminalStream>(std::move(terminal), style, std::move(synchronization), settings);
}

auto TerminalStream::createStandardStreams(TerminalPtr terminal) -> std::pair<TerminalStreamPtr, TerminalStreamPtr> {
    auto errorAttributes = BlockAttributes{};
    errorAttributes.setBold(true);

    auto synchronization = createSynchronization();
    auto outputStream = create(terminal, BlockStyle::reset(), synchronization);
    auto errorStream =
        create(std::move(terminal), BlockStyle{Color{fg::BrightRed, bg::Default}, errorAttributes}, synchronization);
    return {std::move(outputStream), std::move(errorStream)};
}

auto TerminalStream::encoding() const noexcept -> StringEncoding {
    return StringEncoding::Utf8;
}

auto TerminalStream::effectiveEncoding() const noexcept -> StringEncoding {
    return StringEncoding::Utf8;
}

auto TerminalStream::outputSettings() const noexcept -> const OutputStreamSettings & {
    return _data->settings;
}

auto TerminalStream::state() const noexcept -> StreamState {
    return _data->streamState.load();
}

auto TerminalStream::isReady() const noexcept -> bool {
    const auto lock = std::scoped_lock{_data->mutex};
    return _data->pendingBytes <= stream::impl::streamBufferSizes(_data->settings.buffering()).ioRing.toSizeT() &&
        state() == StreamState::Open;
}

auto TerminalStream::waitForReady() -> StreamWaitStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto ready = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this]() -> bool {
        return _data->pendingBytes <= stream::impl::streamBufferSizes(_data->settings.buffering()).ioRing.toSizeT() ||
            _data->error || state() != StreamState::Open;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return ready && state() == StreamState::Open ? StreamWaitStatus::Ready : StreamWaitStatus::Timeout;
}

auto TerminalStream::flush() -> StreamWriteStatus {
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

auto TerminalStream::close() -> StreamCloseStatus {
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

void TerminalStream::abort() noexcept {
    _data->aborted.store(true);
    _data->streamState.store(StreamState::Closed);
    if (_data->mutex.try_lock()) {
        _data->queue.clear();
        _data->pendingBytes = 0U;
        _data->mutex.unlock();
    }
    _data->condition.notify_all();
}

auto TerminalStream::write(const Char character) -> StreamWriteStatus {
    return write(String::fromCharacter(character.isValidUnicode() ? character : Char::replacement()));
}

auto TerminalStream::write(const String &text) -> StreamWriteStatus {
    auto command = impl::TerminalStreamData::Command{text, {}, false};
    auto lock = std::unique_lock{_data->mutex};
    command.style = _data->style;
    const auto byteLength = impl::TerminalStreamData::commandLength(command);
    const auto capacity = stream::impl::streamBufferSizes(_data->settings.buffering()).ioRing.toSizeTOrThrow() +
        _data->settings.backBufferLimit().toSizeTOrThrow();
    if (byteLength > capacity) {
        throwError(
            "Failed to write to the terminal."_el,
            "The complete terminal write exceeds the configured output buffer limit."_el);
    }
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
        throwError("Failed to write to the terminal."_el, "The terminal stream is not open."_el);
    }
    _data->queue.push_back(std::move(command));
    _data->pendingBytes += byteLength;
    _data->schedule();
    return StreamWriteStatus::Success;
}

auto TerminalStream::writeLine() -> StreamWriteStatus {
    return writeLine(String{});
}

auto TerminalStream::writeLine(const String &text) -> StreamWriteStatus {
    auto command = impl::TerminalStreamData::Command{text, {}, true};
    auto lock = std::unique_lock{_data->mutex};
    command.style = _data->style;
    const auto byteLength = impl::TerminalStreamData::commandLength(command);
    const auto capacity = stream::impl::streamBufferSizes(_data->settings.buffering()).ioRing.toSizeTOrThrow() +
        _data->settings.backBufferLimit().toSizeTOrThrow();
    if (byteLength > capacity) {
        throwError(
            "Failed to write a terminal line."_el,
            "The complete terminal line exceeds the configured output buffer limit."_el);
    }
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
        throwError("Failed to write a terminal line."_el, "The terminal stream is not open."_el);
    }
    _data->queue.push_back(std::move(command));
    _data->pendingBytes += byteLength;
    _data->schedule();
    return StreamWriteStatus::Success;
}

auto TerminalStream::style() const -> BlockStyle {
    const auto lock = std::scoped_lock{_data->mutex};
    return _data->style;
}

void TerminalStream::setStyle(const BlockStyle style) {
    const auto lock = std::scoped_lock{_data->mutex};
    _data->style = style;
}

}
