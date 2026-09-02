// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConsoleLogWriterOptions_fwd.hpp"
#include "FileLogWriterOptions_fwd.hpp"
#include "LogEntry.hpp"
#include "LogWriter_fwd.hpp"
#include "SyslogLogWriterOptions_fwd.hpp"

#include "impl/LogManagerData_fwd.hpp"
#include "line/LogLine_fwd.hpp"

#include "../cterm/Terminal_fwd.hpp"

#include <mutex>
#include <span>

namespace erbsland::log {

/// Interface for background log targets.
/// @tested{LogWriterTest}
class LogWriter {
    friend class impl::LogManagerData;

public:
    /// One immutable entry and formatted line in a writer batch.
    /// @tested{LogCoreTest}
    class BatchItem final {
    public:
        /// Create a batch item sharing immutable entry and line values.
        /// @param entry The nonempty immutable entry to deliver.
        /// @param line The nonempty formatted line associated with `entry`.
        BatchItem(LogEntryConstPtr entry, LogLineConstPtr line);

        /// Access the shared immutable entry.
        /// @return The nonempty producer-created entry pointer.
        [[nodiscard]] auto entry() const noexcept -> const LogEntryConstPtr & { return _entry; }
        /// Access the shared formatted line.
        /// @return The nonempty worker-formatted line pointer.
        [[nodiscard]] auto line() const noexcept -> const LogLineConstPtr & { return _line; }

    private:
        LogEntryConstPtr _entry; ///< Shared immutable entry.
        LogLineConstPtr _line;   ///< Shared immutable formatted line.
    };

    /// A contiguous batch of shared immutable entry and line pointers.
    /// The span itself is valid only for the duration of `writeBatch()`. A writer can safely retain an entry or line
    /// beyond the call by copying its shared pointer.
    using Batch = std::span<const BatchItem>;

public:
    /// Create the built-in console writer.
    /// @param terminal The terminal receiving complete log paragraphs.
    /// @return A writer that renders styled paragraphs to `terminal`.
    [[nodiscard]] static auto createForConsole(cterm::TerminalPtr terminal) -> LogWriterPtr;
    /// Create the built-in console writer.
    /// @param terminal The terminal receiving complete log paragraphs.
    /// @param options Style and paragraph layout settings.
    /// @return A writer that renders styled paragraphs to `terminal`.
    [[nodiscard]] static auto createForConsole(cterm::TerminalPtr terminal, const ConsoleLogWriterOptions &options)
        -> LogWriterPtr;
    /// Create the built-in resilient file writer.
    /// @param options The target path, open mode, rotation, and retention settings.
    /// @return A writer that persists formatted lines to the configured file.
    [[nodiscard]] static auto createForFile(const FileLogWriterOptions &options) -> LogWriterPtr;
    /// Create the built-in syslog writer with default UDP settings.
    /// @return A writer that sends RFC 5424 messages to the default syslog endpoint.
    [[nodiscard]] static auto createForSyslog() -> LogWriterPtr;
    /// Create the built-in syslog writer.
    /// @param options The transport, endpoint, RFC 5424 fields, TLS label, and pending-data limit.
    /// @return A writer that sends RFC 5424 messages using the configured transport.
    [[nodiscard]] static auto createForSyslog(const SyslogLogWriterOptions &options) -> LogWriterPtr;

    // defaults
    virtual ~LogWriter() = default;
    /// Deliver one filtered entry and its formatted line.
    /// The manager calls this method only on its private worker and contains any exception it propagates.
    /// @param entry The nonempty shared immutable producer-created entry accepted by the route.
    /// @param line The nonempty shared immutable line formatted from `entry` using the active configuration.
    /// @throws Any implementation-specific delivery exception. The manager contains and counts it.
    virtual void write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) = 0;
    /// Deliver a batch of filtered entries and formatted lines.
    /// The default implementation calls `write()` for every item in order. Writers can override this method to
    /// amortize target checks and system calls over the batch.
    /// @param batch The ordered nonempty batch to deliver during the call.
    /// @throws Any implementation-specific delivery exception. The manager contains and counts it.
    virtual void writeBatch(Batch batch);
    /// Flush any buffered writer data.
    /// @throws Any implementation-specific delivery exception. The manager contains and counts it.
    virtual void flush() {}
    /// Release writer resources before manager shutdown.
    virtual void close() noexcept {}

private:
    /// Exclusively bind this writer to one active manager.
    /// @param manager The identity of the manager requesting the binding.
    /// @return `true` if the writer was unbound or was already bound to `manager`.
    [[nodiscard]] auto bind(const void *manager) noexcept -> bool;
    /// Release a matching manager binding.
    /// @param manager The manager identity whose binding shall be released.
    void release(const void *manager) noexcept;

private:
    std::mutex _bindingMutex; ///< Protects active-manager ownership.
    const void *_manager{};   ///< Identity of the active manager, or null while unbound.
};

}
