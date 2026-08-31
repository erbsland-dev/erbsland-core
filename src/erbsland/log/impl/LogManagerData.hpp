// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogManagerData_fwd.hpp"

#include "../LogConfiguration.hpp"
#include "../LogEntry.hpp"
#include "../LogManagerOptions.hpp"
#include "../LogManagerStatistics.hpp"
#include "../LogStream_fwd.hpp"

#include "../../event/EventThread_fwd.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace erbsland::log::impl {

/// Shared state and worker operations for a log manager.
/// @tested{LogCoreTest LogWriterTest}
class LogManagerData final : public std::enable_shared_from_this<LogManagerData> {
    static constexpr auto cMaximumBatchEntries = std::size_t{256U}; ///< Maximum entries delivered per writer call.

private:
    /// One queued entry with the configuration snapshot selected at enqueue time.
    /// @tested{LogCoreTest}
    struct QueuedEntry final {
        LogEntryConstPtr entry;                                ///< Immutable producer-created entry.
        std::shared_ptr<const LogConfiguration> configuration; ///< Configuration used during delivery.
        unit::ByteLength byteSize;                             ///< Bytes charged against the queue limit.
    };

public:
    /// Create manager data with validated initial limits.
    /// @param options The validated initial queue and shutdown limits.
    explicit LogManagerData(LogManagerOptions options);
    /// Ensure shutdown during destruction.
    ~LogManagerData();

    /// Start the event worker and create the root stream.
    void initialize();
    /// Access the root producer stream.
    [[nodiscard]] auto rootStream() const noexcept -> const LogStreamPtr & { return _rootStream; }
    /// Create a stream.
    /// @param path The validated path represented by the new stream.
    /// @param traceSection The optional trace section controlling trace emission.
    /// @return A new producer stream connected weakly to this manager.
    [[nodiscard]] auto createStream(LogPath path, LogTraceSection traceSection) -> LogStreamPtr;
    /// Capture, sanitize, limit, and enqueue one producer message.
    /// @param level The severity assigned by the producer.
    /// @param timestamp The UTC timestamp captured before producer-side formatting.
    /// @param path The path of the producing stream.
    /// @param message The formatted message to sanitize and enqueue.
    void enqueue(LogLevel level, time::DateTime timestamp, const LogPath &path, text::String message) noexcept;
    /// Synchronously install a replacement configuration.
    /// @param configuration The complete replacement configuration.
    /// @throws err::LogicError If shutdown started or a writer is active in another manager.
    void setConfiguration(LogConfiguration configuration);
    /// Install a writer retained across ordinary configuration replacements.
    /// @param writer The writer to preserve in later configuration snapshots.
    /// @param filter The route filter applied to the persistent writer.
    /// @throws err::LogicError If shutdown started, the writer is empty, or it belongs to another manager.
    void addPersistentWriter(LogWriterPtr writer, LogWriterFilter filter);
    /// Copy the active configuration.
    /// @return A value snapshot of the current configuration.
    [[nodiscard]] auto configuration() const -> LogConfiguration;
    /// Pause ordinary queue draining.
    void pause() noexcept;
    /// Resume queue draining.
    void resume() noexcept;
    /// Drain within the configured deadline and stop the worker.
    void shutdown() noexcept;
    /// Read current manager counters and queue usage.
    /// @return A point-in-time statistics snapshot.
    [[nodiscard]] auto statistics() const noexcept -> LogManagerStatistics;

private:
    /// Apply the producer-side byte limit and truncation mark.
    /// @param message The sanitized message to limit.
    /// @param maximumBytes The maximum retained UTF-8 byte count.
    /// @return The original message or its valid-code-point truncated representation.
    [[nodiscard]] static auto truncatedMessage(text::String message, unit::ByteLength maximumBytes) -> text::String;
    /// Test whether the bounded queue can accept an entry.
    /// @param level The entry severity selecting ordinary or reserved capacity.
    /// @param byteSize The number of bytes charged for the entry.
    /// @return `true` if both entry-count and byte limits have capacity.
    [[nodiscard]] auto canAccept(LogLevel level, unit::ByteLength byteSize) const noexcept -> bool;
    /// Test whether a paused queue has entered its reserved capacity.
    /// @return `true` if ordinary entry or byte capacity is exhausted.
    [[nodiscard]] auto pressureReached() const noexcept -> bool;
    /// Schedule one worker drain operation while holding the manager mutex.
    void scheduleLocked() noexcept;
    /// Process queued entries on the private event worker.
    void drain();
    /// Format and deliver one same-configuration batch.
    /// @param batch The entries removed from the queue for this transaction.
    void processBatch(std::vector<QueuedEntry> &batch);
    /// Recompute all live stream trace flags while holding the manager mutex.
    void refreshTraceFlagsLocked();
    /// Collect unique writer instances from a configuration.
    /// @param configuration The configuration whose bindings shall be inspected.
    /// @return Each distinct nonempty writer in first-binding order.
    [[nodiscard]] static auto uniqueWriters(const LogConfiguration &configuration) -> std::vector<LogWriterPtr>;
    /// Test whether a writer vector contains the given instance.
    /// @param writers The writer vector to search.
    /// @param writer The shared writer instance to find.
    /// @return `true` if the vector contains the same writer instance.
    [[nodiscard]] static auto containsWriter(
        const std::vector<LogWriterPtr> &writers, const LogWriterPtr &writer) noexcept -> bool;
    /// Close writer instances on the event worker.
    /// @param writers The removed writer instances to close and release.
    void closeWriters(const std::vector<LogWriterPtr> &writers);
    /// Close and release every active writer during shutdown.
    void stopWriters() noexcept;

private:
    LogManagerOptions _options;                  ///< Active queue and shutdown limits.
    event::UnmanagedEventThreadPtr _eventThread; ///< Private worker event loop.
    mutable std::mutex _mutex;                   ///< Protects queue, configuration, and worker state.
    std::condition_variable _condition;          ///< Signals processing, reconfiguration, and shutdown state.
    std::deque<QueuedEntry> _queue;              ///< Bounded FIFO of entries awaiting delivery.
    unit::ByteLength _queuedBytes;               ///< Bytes currently charged to the queue.
    std::shared_ptr<const LogConfiguration> _configuration; ///< Active immutable configuration snapshot.
    std::vector<LogWriterBinding> _persistentWriters;       ///< Bindings retained across configuration replacement.
    LogStreamPtr _rootStream;                               ///< Root producer stream.
    std::vector<std::weak_ptr<LogStream>> _streams;         ///< Live streams whose trace caches need refresh.
    bool _paused{false};                                    ///< Whether ordinary draining is paused.
    bool _scheduled{false};                                 ///< Whether a drain call is queued or running.
    bool _processing{false};                                ///< Whether a writer transaction is in progress.
    bool _reconfiguring{false};                             ///< Whether a synchronous configuration barrier is active.
    bool _shuttingDown{false};                              ///< Whether graceful shutdown has started.
    bool _stopped{false};                                   ///< Whether the worker reached its terminal state.
    std::atomic<uint64_t> _nextSequence{1U};                ///< Sequence assigned to the next producer entry.
    std::atomic<uint64_t> _acceptedEntries{};               ///< Entries accepted into the queue.
    std::atomic<uint64_t> _writtenEntries{};                ///< Entries delivered to at least one writer.
    std::atomic<uint64_t> _droppedEntries{};                ///< Entries rejected or discarded.
    std::atomic<uint64_t> _writerFailures{};                ///< Writer exceptions contained by the worker.
};

}
