// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalStream.hpp"

#include "Terminal.hpp"

#include "../stream/impl/IoService.hpp"
#include "../text/Literals.hpp"
#include "../time/TimePoint.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <mutex>
#include <utility>

namespace erbsland::cterm {

using namespace text::literals;

class TerminalStream::Data final : public std::enable_shared_from_this<Data> {
public:
    struct Command final {
        text::String text;
        BlockStyle style;
        bool lineBreak{false};
    };

public:
    Data(
        TerminalPtr streamTerminal,
        BlockStyle streamStyle,
        TerminalStreamSynchronizationPtr streamSynchronization,
        stream::OutputStreamSettings streamSettings) :
        terminal{std::move(streamTerminal)},
        style{streamStyle},
        synchronization{std::move(streamSynchronization)},
        settings{streamSettings} {
        if (terminal == nullptr) {
            streamState.store(stream::StreamState::Closed);
        }
    }

    void schedule() {
        if (workInProgress || aborted.load()) {
            return;
        }
        if (queue.empty()) {
            if (streamState.load() == stream::StreamState::Closing) {
                scheduleFlush(true, 0U);
            }
            return;
        }
        workInProgress = true;
        auto self = shared_from_this();
        auto command = queue.front();
        stream::impl::IoService::submitIoWork(
            [self = std::move(self), command = std::move(command)] { self->performWrite(command); });
    }

    void performWrite(const Command &command) {
        auto failure = std::exception_ptr{};
        try {
            const auto terminalLock = std::scoped_lock{synchronization->_mutex};
            terminal->setStyle(command.style);
            try {
                if (!command.text.isEmpty()) {
                    terminal->write(command.text);
                }
                if (command.lineBreak) {
                    terminal->writeLineBreak();
                }
                terminal->setStyle(BlockStyle::reset());
            } catch (...) {
                terminal->setStyle(BlockStyle::reset());
                throw;
            }
        } catch (...) {
            failure = std::current_exception();
        }
        {
            const auto lock = std::scoped_lock{mutex};
            workInProgress = false;
            if (failure) {
                error = failure;
                streamState.store(stream::StreamState::Failed);
            } else if (!aborted.load()) {
                pendingBytes -= commandLength(command);
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
        stream::impl::IoService::submitIoWork(
            [self = std::move(self), closeAfterFlush, generation] { self->performFlush(closeAfterFlush, generation); });
    }

    void performFlush(const bool closeAfterFlush, const uint64_t generation) {
        auto failure = std::exception_ptr{};
        try {
            const auto terminalLock = std::scoped_lock{synchronization->_mutex};
            terminal->flush();
        } catch (...) {
            failure = std::current_exception();
        }
        {
            const auto lock = std::scoped_lock{mutex};
            workInProgress = false;
            if (failure) {
                error = failure;
                streamState.store(stream::StreamState::Failed);
            } else if (closeAfterFlush) {
                streamState.store(stream::StreamState::Closed);
            } else {
                completedFlushGeneration = generation;
                schedule();
            }
        }
        condition.notify_all();
    }

    [[nodiscard]] static auto commandLength(const Command &command) noexcept -> std::size_t {
        return command.text.length().toSizeT() + (command.lineBreak ? 1U : 0U);
    }

public:
    TerminalPtr terminal;
    BlockStyle style;
    TerminalStreamSynchronizationPtr synchronization;
    stream::OutputStreamSettings settings;
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::deque<Command> queue;
    std::size_t pendingBytes{0U};
    std::atomic<stream::StreamState> streamState{stream::StreamState::Open};
    std::atomic<bool> aborted{false};
    bool workInProgress{false};
    uint64_t flushGeneration{0U};
    uint64_t completedFlushGeneration{0U};
    std::exception_ptr error;
};

TerminalStream::TerminalStream(
    TerminalPtr terminal,
    const BlockStyle style,
    TerminalStreamSynchronizationPtr synchronization,
    const stream::OutputStreamSettings settings) :
    _terminal{std::move(terminal)} {
    if (synchronization == nullptr) {
        synchronization = createSynchronization();
    }
    _data = std::make_shared<Data>(_terminal, style, std::move(synchronization), settings);
}

auto TerminalStream::createSynchronization() -> TerminalStreamSynchronizationPtr {
    return std::make_shared<TerminalStreamSynchronization>();
}

auto TerminalStream::create(
    TerminalPtr terminal,
    const BlockStyle style,
    TerminalStreamSynchronizationPtr synchronization,
    const stream::OutputStreamSettings settings) -> TerminalStreamPtr {
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

auto TerminalStream::encoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto TerminalStream::effectiveEncoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto TerminalStream::outputSettings() const noexcept -> const stream::OutputStreamSettings & {
    return _data->settings;
}

auto TerminalStream::state() const noexcept -> stream::StreamState {
    return _data->streamState.load();
}

auto TerminalStream::isReady() const noexcept -> bool {
    const auto lock = std::scoped_lock{_data->mutex};
    return _data->pendingBytes <= _data->settings.bufferCapacity().toSizeT() && state() == stream::StreamState::Open;
}

auto TerminalStream::waitForReady() -> stream::StreamWaitStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto ready = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this] {
        return _data->pendingBytes <= _data->settings.bufferCapacity().toSizeT() || _data->error ||
            state() != stream::StreamState::Open;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return ready && state() == stream::StreamState::Open ? stream::StreamWaitStatus::Ready
                                                         : stream::StreamWaitStatus::Timeout;
}

auto TerminalStream::flush() -> stream::StreamWriteStatus {
    auto lock = std::unique_lock{_data->mutex};
    const auto deadline = time::TimePoint::inFuture(_data->settings.timeout());
    const auto drained = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this] {
        return (_data->queue.empty() && !_data->workInProgress) || _data->error || state() != stream::StreamState::Open;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    if (!drained || state() != stream::StreamState::Open) {
        return stream::StreamWriteStatus::Timeout;
    }
    const auto generation = ++_data->flushGeneration;
    _data->scheduleFlush(false, generation);
    const auto flushed = _data->condition.wait_until(lock, deadline.toStdTimePoint(), [this, generation] {
        return _data->completedFlushGeneration >= generation || _data->error;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return flushed ? stream::StreamWriteStatus::Success : stream::StreamWriteStatus::Timeout;
}

auto TerminalStream::close() -> stream::StreamCloseStatus {
    auto lock = std::unique_lock{_data->mutex};
    if (state() == stream::StreamState::Closed) {
        return stream::StreamCloseStatus::Closed;
    }
    if (state() == stream::StreamState::Open) {
        _data->streamState.store(stream::StreamState::Closing);
        _data->schedule();
    }
    const auto closed = _data->condition.wait_for(lock, _data->settings.timeout().toStdNanoseconds(), [this] {
        return state() == stream::StreamState::Closed || state() == stream::StreamState::Failed;
    });
    if (_data->error) {
        std::rethrow_exception(_data->error);
    }
    return closed ? stream::StreamCloseStatus::Closed : stream::StreamCloseStatus::Timeout;
}

void TerminalStream::abort() noexcept {
    _data->aborted.store(true);
    _data->streamState.store(stream::StreamState::Closed);
    if (_data->mutex.try_lock()) {
        _data->queue.clear();
        _data->pendingBytes = 0U;
        _data->mutex.unlock();
    }
    _data->condition.notify_all();
}

auto TerminalStream::write(const text::Char character) -> stream::StreamWriteStatus {
    return write(text::String::fromCharacter(character.isValidUnicode() ? character : text::Char::replacement()));
}

auto TerminalStream::write(const text::StringView &text) -> stream::StreamWriteStatus {
    auto command = Data::Command{text::String{text}, {}, false};
    auto lock = std::unique_lock{_data->mutex};
    command.style = _data->style;
    const auto byteLength = Data::commandLength(command);
    const auto capacity =
        _data->settings.bufferCapacity().toSizeTOrThrow() + _data->settings.backBufferLimit().toSizeTOrThrow();
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
            return stream::StreamWriteStatus::Timeout;
        }
    }
    if (state() != stream::StreamState::Open) {
        throwError("Failed to write to the terminal."_el, "The terminal stream is not open."_el);
    }
    _data->queue.push_back(std::move(command));
    _data->pendingBytes += byteLength;
    _data->schedule();
    return stream::StreamWriteStatus::Success;
}

auto TerminalStream::writeLine() -> stream::StreamWriteStatus {
    return writeLine(text::StringView{});
}

auto TerminalStream::writeLine(const text::StringView &text) -> stream::StreamWriteStatus {
    auto command = Data::Command{text::String{text}, {}, true};
    auto lock = std::unique_lock{_data->mutex};
    command.style = _data->style;
    const auto byteLength = Data::commandLength(command);
    const auto capacity =
        _data->settings.bufferCapacity().toSizeTOrThrow() + _data->settings.backBufferLimit().toSizeTOrThrow();
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
            return stream::StreamWriteStatus::Timeout;
        }
    }
    if (state() != stream::StreamState::Open) {
        throwError("Failed to write a terminal line."_el, "The terminal stream is not open."_el);
    }
    _data->queue.push_back(std::move(command));
    _data->pendingBytes += byteLength;
    _data->schedule();
    return stream::StreamWriteStatus::Success;
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
