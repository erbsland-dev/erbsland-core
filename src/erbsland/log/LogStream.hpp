// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLevel.hpp"
#include "LogPath.hpp"
#include "LogStream_fwd.hpp"
#include "LogTraceSection.hpp"

#include "impl/LogManagerData_fwd.hpp"

#include "../stream/AnyStringBuilderStream.hpp"
#include "../time/DateTime.hpp"

#include <atomic>

namespace erbsland::log {

/// A lightweight producer endpoint for one hierarchical log path.
/// @tested{LogCoreTest}
class LogStream final {
    friend class impl::LogManagerData;

private:
    /// Restricts stream construction to manager data.
    class ConstructionToken final {
        friend class impl::LogManagerData;

        /// Create a private construction token.
        ConstructionToken() = default;
    };

public:
    /// Internal constructor used by the manager.
    /// @param path The validated path represented by this stream.
    /// @param traceSection The optional trace configuration section.
    /// @param manager Weak ownership of the manager receiving entries.
    /// @param token The construction token supplied by the manager.
    LogStream(LogPath path, LogTraceSection traceSection, impl::LogManagerDataWeakPtr manager, ConstructionToken token);

    // defaults/deletions
    ~LogStream() = default;
    LogStream(const LogStream &) = delete;
    LogStream(LogStream &&) = delete;
    auto operator=(const LogStream &) -> LogStream & = delete;
    auto operator=(LogStream &&) -> LogStream & = delete;

public:
    /// Access the stream path.
    [[nodiscard]] auto path() const noexcept -> const LogPath & { return _path; }
    /// Access the optional trace section.
    [[nodiscard]] auto traceSection() const noexcept -> const LogTraceSection & { return _traceSection; }
    /// Read the cached trace-enabled flag with one atomic operation.
    [[nodiscard]] auto traceEnabled() const noexcept -> bool { return _traceEnabled.load(std::memory_order_relaxed); }

public:
    /// Emit a trace entry when tracing is currently enabled.
    /// @param args Printable values passed in order to the text-print system.
    template <typename... tArgs>
    void trace(const tArgs &...args) {
        if (traceEnabled()) {
            emit(LogLevel::Trace, args...);
        }
    }

    /// Emit an informational entry.
    /// @param args Printable values passed in order to the text-print system.
    template <typename... tArgs>
    void info(const tArgs &...args) {
        emit(LogLevel::Information, args...);
    }

    /// Emit a warning entry.
    /// @param args Printable values passed in order to the text-print system.
    template <typename... tArgs>
    void warn(const tArgs &...args) {
        emit(LogLevel::Warning, args...);
    }

    /// Emit an error entry.
    /// @param args Printable values passed in order to the text-print system.
    template <typename... tArgs>
    void error(const tArgs &...args) {
        emit(LogLevel::Error, args...);
    }

private:
    /// Format arbitrary printable arguments and emit one entry.
    /// @param level The severity to assign to the entry.
    /// @param args Printable values passed in order to the text-print system.
    template <typename... tArgs>
    void emit(const LogLevel level, const tArgs &...args) {
        const auto timestamp = time::DateTime::now();
        const auto builder = stream::AnyStringBuilderStream::create();
        builder->print(args...);
        emitText(level, timestamp, builder->takeString());
    }

    /// Sanitize and enqueue an already formatted message with its producer timestamp.
    /// @param level The severity to assign to the entry.
    /// @param timestamp The UTC timestamp captured before producer-side formatting.
    /// @param message The formatted message to sanitize and enqueue.
    void emitText(LogLevel level, time::DateTime timestamp, text::String message);
    /// Update the cached trace-enabled flag.
    /// @param enabled The newly computed trace state.
    void setTraceEnabled(bool enabled) noexcept { _traceEnabled.store(enabled, std::memory_order_relaxed); }

private:
    LogPath _path;                          ///< Validated path represented by this stream.
    LogTraceSection _traceSection;          ///< Optional trace configuration section.
    impl::LogManagerDataWeakPtr _manager;   ///< Manager receiving emitted entries.
    std::atomic<bool> _traceEnabled{false}; ///< Cached producer-side trace state.
};

}
