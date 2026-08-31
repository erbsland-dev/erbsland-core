// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogEntry_fwd.hpp"
#include "LogLevel.hpp"
#include "LogPath.hpp"

#include "../text/String.hpp"
#include "../time/DateTime.hpp"

#include <cstdint>

namespace erbsland::log {

/// One immutable log entry created by a producer.
/// @tested{LogCoreTest LogWriterTest}
class LogEntry final {
public:
    /// Create an immutable entry from producer-captured values.
    /// @param sequence The monotonically increasing sequence assigned by the manager.
    /// @param timestamp The UTC timestamp captured before formatting the producer message.
    /// @param level The severity selected by the producer.
    /// @param path The path of the producing stream.
    /// @param message The sanitized message text.
    /// @param truncated Whether the producer-side message-size limit truncated the message.
    LogEntry(
        uint64_t sequence,
        time::DateTime timestamp,
        LogLevel level,
        LogPath path,
        text::String message,
        bool truncated = false);

    // defaults
    ~LogEntry() = default;
    LogEntry(const LogEntry &) = default;
    LogEntry(LogEntry &&) noexcept = default;
    auto operator=(const LogEntry &) -> LogEntry & = default;
    auto operator=(LogEntry &&) noexcept -> LogEntry & = default;

public:
    /// Get the manager-assigned sequence number.
    [[nodiscard]] auto sequence() const noexcept -> uint64_t { return _sequence; }
    /// Get the retained UTC creation timestamp.
    [[nodiscard]] auto timestamp() const noexcept -> const time::DateTime & { return _timestamp; }
    /// Get the entry severity level.
    [[nodiscard]] auto level() const noexcept -> LogLevel { return _level; }
    /// Get the producing stream path.
    [[nodiscard]] auto path() const noexcept -> const LogPath & { return _path; }
    /// Get the sanitized message text.
    [[nodiscard]] auto message() const noexcept -> const text::String & { return _message; }
    /// Test whether the producer-side size limit truncated the message.
    [[nodiscard]] auto isTruncated() const noexcept -> bool { return _truncated; }

private:
    uint64_t _sequence{};                   ///< Manager-assigned entry sequence.
    time::DateTime _timestamp;              ///< Producer-captured UTC timestamp.
    LogLevel _level{LogLevel::Information}; ///< Entry severity.
    LogPath _path;                          ///< Path of the producing stream.
    text::String _message;                  ///< Sanitized message text.
    bool _truncated{};                      ///< Whether the producer truncated the message.
};

}
