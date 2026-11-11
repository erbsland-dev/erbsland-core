// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogManagerData.hpp"

#include "ActiveLogConfiguration.hpp"
#include "LogLineFormatter.hpp"

#include "../line/LogLine.hpp"
#include "../LogStream.hpp"
#include "../LogWriter.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../event/EventTarget.hpp"
#include "../../event/UnmanagedEventThread.hpp"
#include "../../text/Char.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringJoins.hpp"
#include "../../text/StringSide.hpp"
#include "../../time/TimePoint.hpp"
#include "../../time/Timestamp.hpp"

#include <algorithm>
#include <exception>
#include <unordered_map>
#include <utility>

namespace erbsland::log::impl {

using namespace err;
using namespace text;
using namespace text::literals;

LogManagerData::LogManagerData(LogManagerOptions options) {
    _configuration.setManagerOptions(std::move(options));
    _activeConfiguration = std::make_shared<ActiveLogConfiguration>(_configuration);
}

LogManagerData::~LogManagerData() {
    shutdown();
}

void LogManagerData::initialize() {
    _eventThread = event::UnmanagedEventThread::create();
    _eventThread->start();
    _rootStream = createStream(LogPath{}, LogTraceSection{});
}

auto LogManagerData::createStream(LogPath path, LogTraceSection traceSection) -> LogStreamPtr {
    auto stream = std::make_shared<LogStream>(
        std::move(path), std::move(traceSection), weak_from_this(), LogStream::PrivateTag{});
    const auto lock = std::scoped_lock{_mutex};
    stream->setTraceEnabled(_activeConfiguration->isTraceSectionEnabled(stream->traceSection()));
    _streams.emplace_back(stream);
    return stream;
}

void LogManagerData::enqueue(
    const LogLevel level, time::Timestamp timestamp, const LogPath &path, String message) noexcept {
    auto options = LogManagerOptions{};
    {
        const auto lock = std::scoped_lock{_mutex};
        options = _activeConfiguration->managerOptions();
    }
    const auto truncated = message.length() > options.maximumMessageBytes();
    message = truncatedMessage(std::move(message), options.maximumMessageBytes());
    const auto sequence = _nextSequence.fetch_add(1U, std::memory_order_relaxed);
    const auto byteSize = message.length() + path.value().length() + unit::ByteCount{sizeof(LogEntry)};
    const auto lock = std::scoped_lock{_mutex};
    if (_shuttingDown || _stopped || !canAccept(level, byteSize)) {
        ++_droppedEntries;
        return;
    }
    _queue.push_back(
        QueuedEntry{
            std::make_shared<LogEntry>(sequence, timestamp, level, path, std::move(message), truncated),
            _activeConfiguration,
            byteSize});
    _queuedBytes += byteSize;
    ++_acceptedEntries;
    if (!_paused || pressureReached()) {
        scheduleLocked();
    }
}

void LogManagerData::setConfiguration(LogConfiguration configuration) {
    {
        const auto lock = std::scoped_lock{_mutex};
        for (const auto &binding : _persistentWriters) {
            auto present = false;
            for (const auto &configured : configuration.writerBindings()) {
                if (configured.writer == binding.writer) {
                    present = true;
                    break;
                }
            }
            if (!present) {
                configuration.addWriter(binding.writer, binding.filter);
            }
        }
    }
    auto next = std::make_shared<ActiveLogConfiguration>(configuration);
    const auto nextWriters = next->uniqueWriters();
    auto previousWriters = std::vector<LogWriterPtr>{};
    {
        auto lock = std::unique_lock{_mutex};
        _condition.wait(lock, [this]() -> bool { return !_reconfiguring; });
        if (_stopped || _shuttingDown) {
            throw LogicFailure{"A stopped log manager cannot be configured."_el};
        }
        previousWriters = _activeConfiguration->uniqueWriters();
        _reconfiguring = true;
    }
    auto claimed = std::vector<LogWriterPtr>{};
    for (const auto &writer : nextWriters) {
        if (!writer->bind(this)) {
            for (const auto &rollback : claimed) {
                if (!containsWriter(previousWriters, rollback)) {
                    rollback->release(this);
                }
            }
            {
                const auto lock = std::scoped_lock{_mutex};
                _reconfiguring = false;
                if (!_queue.empty() && (!_paused || pressureReached())) {
                    scheduleLocked();
                }
            }
            _condition.notify_all();
            throw LogicFailure{"A log writer is already active in another manager."_el};
        }
        claimed.push_back(writer);
    }

    {
        auto lock = std::unique_lock{_mutex};
        _condition.wait(lock, [this]() -> bool { return !_processing; });
        try {
            if (_stopped || _shuttingDown) {
                throw LogicFailure{"A stopped log manager cannot be configured."_el};
            }
            _configuration = std::move(configuration);
            _activeConfiguration = std::move(next);
            for (auto &queued : _queue) {
                queued.activeConfiguration = _activeConfiguration;
            }
            refreshTraceFlagsLocked();
            _reconfiguring = false;
            if (!_queue.empty() && (!_paused || pressureReached())) {
                scheduleLocked();
            }
            lock.unlock();
            _condition.notify_all();
        } catch (...) {
            _reconfiguring = false;
            if (!_queue.empty() && (!_paused || pressureReached())) {
                scheduleLocked();
            }
            for (const auto &writer : claimed) {
                if (!containsWriter(previousWriters, writer)) {
                    writer->release(this);
                }
            }
            lock.unlock();
            _condition.notify_all();
            throw;
        }
    }
    auto removedWriters = std::vector<LogWriterPtr>{};
    for (const auto &writer : previousWriters) {
        if (!containsWriter(nextWriters, writer)) {
            removedWriters.push_back(writer);
        }
    }
    closeWriters(removedWriters);
}

void LogManagerData::addPersistentWriter(LogWriterPtr writer, LogWriterFilter filter) {
    if (!writer) {
        throw LogicFailure{"A persistent log writer must not be empty."_el};
    }
    auto configuration = LogConfiguration{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_stopped || _shuttingDown) {
            throw LogicFailure{"A stopped log manager cannot be configured."_el};
        }
        for (const auto &binding : _persistentWriters) {
            if (binding.writer == writer) {
                return;
            }
        }
        _persistentWriters.push_back(LogWriterBinding{writer, filter, std::nullopt});
        configuration = _configuration;
    }
    try {
        setConfiguration(std::move(configuration));
    } catch (...) {
        const auto lock = std::scoped_lock{_mutex};
        _persistentWriters.erase(
            std::remove_if(
                _persistentWriters.begin(),
                _persistentWriters.end(),
                [&writer](const LogWriterBinding &binding) -> bool { return binding.writer == writer; }),
            _persistentWriters.end());
        throw;
    }
}

auto LogManagerData::configuration() const -> LogConfiguration {
    const auto lock = std::scoped_lock{_mutex};
    return _configuration;
}

void LogManagerData::pause() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    if (!_shuttingDown && !_stopped) {
        _paused = true;
    }
}

void LogManagerData::resume() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    _paused = false;
    if (!_queue.empty()) {
        scheduleLocked();
    }
}

void LogManagerData::shutdown() noexcept {
    if (!_eventThread) {
        return;
    }
    auto lock = std::unique_lock{_mutex};
    if (!_stopped) {
        _shuttingDown = true;
        _paused = false;
        scheduleLocked();
        const auto deadline = time::TimePoint::inFuture(_activeConfiguration->managerOptions().shutdownTimeout());
        if (!_condition.wait_until(lock, deadline.toStdTimePoint(), [this]() -> bool { return _stopped; })) {
            _droppedEntries += _queue.size();
            _queue.clear();
            _queuedBytes = {};
            scheduleLocked();
            _condition.wait(lock, [this]() -> bool { return _stopped; });
        }
    }
    lock.unlock();
    _eventThread->join();
    _eventThread.reset();
}

auto LogManagerData::statistics() const noexcept -> LogManagerStatistics {
    auto result = LogManagerStatistics{};
    result.acceptedEntries = _acceptedEntries.load();
    result.writtenEntries = _writtenEntries.load();
    result.droppedEntries = _droppedEntries.load();
    result.writerFailures = _writerFailures.load();
    const auto lock = std::scoped_lock{_mutex};
    result.queuedEntries = _queue.size();
    result.queuedBytes = _queuedBytes;
    return result;
}

auto LogManagerData::truncatedMessage(String message, const unit::ByteCount maximumBytes) -> String {
    if (message.length() <= maximumBytes) {
        return message;
    }
    const auto mark = "…"_el;
    if (maximumBytes < mark.length()) {
        return {};
    }
    const static auto cReplacementSet = CharSet{Char::replacement()};
    const auto availableLength = maximumBytes - mark.length();
    const auto truncated = message.slice(StringSide::Front, availableLength).trimmed(cReplacementSet, StringSide::Back);
    return text::join(truncated, mark);
}

auto LogManagerData::canAccept(const LogLevel level, const unit::ByteCount byteSize) const noexcept -> bool {
    const auto &options = _activeConfiguration->managerOptions();
    const auto critical = level == LogLevel::Warning || level == LogLevel::Error;
    auto entryLimit = options.maximumEntries();
    if (!critical) {
        entryLimit -= std::min(entryLimit, options.reservedErrorEntries());
    }
    const auto maximumBytes = options.maximumBytes();
    const auto reservedBytes = std::min(maximumBytes, options.reservedErrorBytes());
    const auto byteLimit = critical ? maximumBytes : maximumBytes - reservedBytes;

    return _queue.size() < entryLimit && byteSize <= byteLimit && _queuedBytes <= byteLimit - byteSize;
}

auto LogManagerData::pressureReached() const noexcept -> bool {
    const auto &options = _activeConfiguration->managerOptions();
    const auto entryThreshold =
        options.maximumEntries() - std::min(options.maximumEntries(), options.reservedErrorEntries());
    const auto maximumBytes = options.maximumBytes();
    const auto byteThreshold = maximumBytes - std::min(maximumBytes, options.reservedErrorBytes());
    return _queue.size() >= entryThreshold || _queuedBytes >= byteThreshold;
}

void LogManagerData::scheduleLocked() noexcept {
    if (_scheduled || _stopped || !_eventThread) {
        return;
    }
    _scheduled = true;
    const auto weak = weak_from_this();
    _eventThread->loop()->invoke([weak]() -> void {
        if (const auto self = weak.lock()) {
            self->drain();
        }
    });
}

void LogManagerData::drain() {
    while (true) {
        auto batch = std::vector<QueuedEntry>{};
        {
            auto lock = std::unique_lock{_mutex};
            if (_reconfiguring && !_shuttingDown) {
                _scheduled = false;
                lock.unlock();
                _condition.notify_all();
                return;
            }
            if (_queue.empty()) {
                _scheduled = false;
                if (_shuttingDown) {
                    _stopped = true;
                    lock.unlock();
                    stopWriters();
                    _eventThread->quit();
                    _condition.notify_all();
                }
                return;
            }
            if (_paused && !_shuttingDown && !pressureReached()) {
                _scheduled = false;
                return;
            }
            const auto activeConfiguration = _queue.front().activeConfiguration;
            batch.reserve(std::min(cMaximumBatchEntries, _queue.size()));
            while (
                !_queue.empty() && batch.size() < cMaximumBatchEntries &&
                _queue.front().activeConfiguration == activeConfiguration) {
                _queuedBytes -= _queue.front().byteSize;
                batch.emplace_back(std::move(_queue.front()));
                _queue.pop_front();
            }
            _processing = true;
        }

        try {
            processBatch(batch);
        } catch (...) {
            const auto lock = std::scoped_lock{_mutex};
            _processing = false;
            _condition.notify_all();
            throw;
        }
        {
            const auto lock = std::scoped_lock{_mutex};
            _processing = false;
        }
        _condition.notify_all();
    }
}

void LogManagerData::processBatch(std::vector<QueuedEntry> &batch) {
    if (batch.empty()) {
        return;
    }
    const auto &active = *batch.front().activeConfiguration;
    auto lines = std::unordered_map<LogLineFormatterConstPtr, std::vector<LogLineConstPtr>>{};
    auto written = std::vector<bool>(batch.size(), false);
    for (const auto &route : active.writers()) {
        try {
            auto &cachedLines = lines[route.formatter];
            auto items = std::vector<LogWriter::BatchItem>{};
            auto indices = std::vector<std::size_t>{};
            items.reserve(batch.size());
            indices.reserve(batch.size());
            for (auto index = std::size_t{}; index < batch.size(); ++index) {
                const auto &entry = batch[index].entry;
                if (!route.filter.accepts(entry->level(), entry->path())) {
                    continue;
                }
                if (cachedLines.empty()) {
                    cachedLines.resize(batch.size());
                }
                auto &line = cachedLines[index];
                if (!line) {
                    line = route.formatter->format(*entry);
                }
                items.emplace_back(entry, line);
                indices.push_back(index);
            }
            if (items.empty()) {
                continue;
            }
            route.writer->writeBatch(items);
            for (const auto index : indices) {
                written[index] = true;
            }
        } catch (const RuntimeError &) {
            ++_writerFailures;
        }
    }
    _writtenEntries += static_cast<uint64_t>(std::ranges::count(written, true));
}

void LogManagerData::refreshTraceFlagsLocked() {
    _streams.erase(
        std::remove_if(
            _streams.begin(),
            _streams.end(),
            [this](const std::weak_ptr<LogStream> &weak) -> bool {
                if (const auto stream = weak.lock()) {
                    stream->setTraceEnabled(_activeConfiguration->isTraceSectionEnabled(stream->traceSection()));
                    return false;
                }
                return true;
            }),
        _streams.end());
}

auto LogManagerData::containsWriter(const std::vector<LogWriterPtr> &writers, const LogWriterPtr &writer) noexcept
    -> bool {
    return std::ranges::find(writers, writer) != writers.end();
}

void LogManagerData::closeWriters(const std::vector<LogWriterPtr> &writers) {
    if (writers.empty()) {
        return;
    }
    auto completionMutex = std::mutex{};
    auto completionCondition = std::condition_variable{};
    auto completed = false;
    _eventThread->loop()->invoke([this, writers, &completionMutex, &completionCondition, &completed]() -> void {
        for (const auto &writer : writers) {
            writer->close();
            writer->release(this);
        }
        {
            const auto lock = std::scoped_lock{completionMutex};
            completed = true;
        }
        completionCondition.notify_one();
    });
    auto lock = std::unique_lock{completionMutex};
    completionCondition.wait(lock, [&completed]() -> bool { return completed; });
}

void LogManagerData::stopWriters() noexcept {
    auto writers = std::vector<LogWriterPtr>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        writers = _activeConfiguration->uniqueWriters();
    }
    for (const auto &writer : writers) {
        try {
            writer->flush();
        } catch (const RuntimeError &) {
            ++_writerFailures;
        }
        writer->close();
        writer->release(this);
    }
}

}
