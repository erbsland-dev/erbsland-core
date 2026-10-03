// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLineFormatter_fwd.hpp"

#include "../line/LogLine.hpp"
#include "../LogEntry.hpp"
#include "../LogLineFormat.hpp"

#include <vector>

namespace erbsland::log::impl {

/// Formats immutable log entries on the manager worker.
/// @tested{LogCoreTest LogLineFormatterTest}
class LogLineFormatter final {
public:
    /// Compile a pattern and retain an independent copy of its formatting settings.
    /// @param settings The complete settings used for every entry formatted by this instance.
    explicit LogLineFormatter(LogLineFormat settings);

public: // formatting
    /// Format one immutable entry using the prepared pattern.
    /// @param entry The immutable entry to format.
    /// @return A shared immutable complete line split into semantic segments for styled writers.
    [[nodiscard]] auto format(const LogEntry &entry) const -> LogLineConstPtr;

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
    /// Parse a validated pattern into literal text and placeholder segments.
    /// @param pattern The validated pattern to compile.
    /// @return Segments containing literal text or a placeholder with empty text.
    [[nodiscard]] static auto compilePattern(const text::String &pattern) -> std::vector<LogLineSegment>;
    /// Expand the compiled pattern into semantic line segments.
    /// @param time The rendered timestamp replacement.
    /// @param level The rendered severity replacement.
    /// @param name The rendered stream-name replacement.
    /// @param message The rendered message replacement.
    /// @return A shared immutable expanded line with semantic segment boundaries retained.
    [[nodiscard]] auto render(
        const text::String &time,
        const text::String &level,
        const text::String &name,
        const text::String &message) const -> LogLineConstPtr;

private:
    LogLineFormat _settings;                      ///< Independently owned formatting settings.
    std::vector<LogLineSegment> _patternSegments; ///< Compiled literals and placeholder identifiers.
};

}
