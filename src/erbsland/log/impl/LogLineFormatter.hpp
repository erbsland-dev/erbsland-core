// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../LogEntry.hpp"
#include "../LogLine.hpp"
#include "../LogLineFormat.hpp"

namespace erbsland::log::impl {

/// Formats immutable log entries on the manager worker.
/// @tested{LogCoreTest}
class LogLineFormatter final {
public:
    /// Create a formatter for one entry and format configuration.
    /// @param entry The immutable entry to format.
    /// @param settings The line-format settings to apply.
    LogLineFormatter(const LogEntry &entry, const LogLineFormat &settings) noexcept :
        _entry{entry}, _settings{settings} {}

    /// Format the configured entry.
    /// @return A shared immutable complete line split into semantic segments for styled writers.
    [[nodiscard]] auto format() const -> LogLineConstPtr;

private:
    /// Render a stream path using the configured name representation.
    /// @param path The stream path to render.
    /// @param settings The name representation and limit settings.
    /// @return The rendered stream name.
    [[nodiscard]] static auto nameText(const LogPath &path, const LogLineFormat &settings) -> text::String;
    /// Apply message-only truncation rules.
    /// @param message The sanitized message to render.
    /// @param settings The selected truncation rule, limit, and marker.
    /// @return The message after first-line or character-count truncation.
    [[nodiscard]] static auto messageText(const text::String &message, const LogLineFormat &settings) -> text::String;
    /// Compute a message that satisfies the configured complete-line limit.
    /// @param line The initially formatted line used to measure non-message content.
    /// @param message The message text before total-line truncation.
    /// @param settings The total-line limit and truncation marker.
    /// @return The message shortened so the complete line fits when possible.
    [[nodiscard]] static auto truncatedMessageForTotalLength(
        const LogLine &line, const text::String &message, const LogLineFormat &settings) -> text::String;
    /// Expand the placeholder pattern into semantic line segments.
    /// @param settings The placeholder pattern to expand.
    /// @param time The rendered timestamp replacement.
    /// @param level The rendered severity replacement.
    /// @param name The rendered stream-name replacement.
    /// @param message The rendered message replacement.
    /// @return A shared immutable expanded line with semantic segment boundaries retained.
    [[nodiscard]] static auto render(
        const LogLineFormat &settings,
        const text::String &time,
        const text::String &level,
        const text::String &name,
        const text::String &message) -> LogLineConstPtr;

private:
    const LogEntry &_entry;         ///< Entry being formatted.
    const LogLineFormat &_settings; ///< Formatting settings to apply.
};

}
