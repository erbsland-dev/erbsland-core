// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../FileLogWriterOptions.hpp"
#include "../LogWriter.hpp"

#include "../../stream/TextOutputStream_fwd.hpp"
#include "../../system/FileIdentity.hpp"
#include "../../time/TimePoint.hpp"

#include <cstddef>
#include <deque>

namespace erbsland::log::impl {

/// Resilient path-stream based file log writer.
/// @tested{LogWriterTest}
class FileLogWriter final : public LogWriter {
public:
    /// Create a writer with the given file settings.
    /// @param options The target path, open mode, rotation, and retention settings.
    explicit FileLogWriter(FileLogWriterOptions options);

public: // implements LogWriter
    ~FileLogWriter() override { close(); }
    void write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) override;
    void writeBatch(Batch batch) override;
    void flush() override;
    void close() noexcept override;

private:
    /// Open the configured target if it is currently closed and retry is due.
    void ensureOpen();
    /// Verify that the target path still identifies the opened file.
    void verifyIdentity();
    /// Test whether the next write requires rotation.
    /// @param entry The entry about to be written.
    /// @param writeSize The encoded byte count of the complete line and line break.
    /// @return `true` if time or size rotation must run before the write.
    [[nodiscard]] auto shouldRotate(const LogEntry &entry, unit::ByteLength writeSize) const -> bool;
    /// Rotate according to the configured mode.
    /// @param entry The entry whose timestamp selects a time-rotation archive.
    void rotate(const LogEntry &entry);
    /// Rotate a file using numbered size archives.
    void rotateBySize();
    /// Rotate a file into a timestamp-derived archive.
    void rotateByTime();
    /// Build an archive path from a fixed suffix key.
    /// @param key The fixed rotation key appended to the configured base path.
    /// @return The derived archive path.
    [[nodiscard]] auto archivePath(const text::String &key) const -> path::Path;
    /// Build the current time-rotation key for an entry.
    /// @param entry The entry whose timestamp selects the rotation period.
    /// @return The fixed hourly, daily, or weekly rotation key.
    [[nodiscard]] auto rotationKey(const LogEntry &entry) const -> text::String;
    /// Close and forget the active stream.
    void closeStream() noexcept;
    /// Advance the bounded retry delay.
    void scheduleRetry() noexcept;

private:
    FileLogWriterOptions _options;       ///< Target path, rotation, and retention settings.
    stream::TextOutputStreamPtr _stream; ///< Currently opened encoded file stream.
    system::FileIdentity _identity;      ///< Identity captured from the opened file.
    unit::ByteLength _currentSize;       ///< Initial file size plus bytes accepted by the stream.
    text::String _rotationKey;           ///< Active time-rotation period key.
    std::deque<path::Path> _archives;    ///< Archives created by this writer in age order.
    time::TimePoint _nextIdentityCheck;  ///< Earliest time for the next periodic identity comparison.
    time::TimePoint _nextRetry;          ///< Earliest time for the next recovery attempt.
    unsigned int _retryStep{};           ///< Current bounded exponential-backoff step.
    bool _openedOnce{false};             ///< Whether the initial open mode was already applied.
};

}
