// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLevelFormat.hpp"
#include "LogMessageTruncation.hpp"
#include "LogNameFormat.hpp"
#include "LogTimestampZone.hpp"

#include "../text/String.hpp"
#include "../unit/CpLength.hpp"

namespace erbsland::log {

/// Formatting settings shared by log writers.
/// @tested{LogCoreTest}
class LogLineFormat final {
public:
    /// Create the default `{time} {level} - {message}` format.
    LogLineFormat();
    /// Get the placeholder pattern.
    [[nodiscard]] auto pattern() const noexcept -> const text::String & { return _pattern; }
    /// Validate and set the placeholder pattern.
    /// @param value The pattern containing literals, escaped braces, and supported placeholders.
    /// @return These settings for chained configuration.
    auto setPattern(text::String value) -> LogLineFormat &;
    /// Get the timestamp rendering zone.
    [[nodiscard]] auto timestampZone() const noexcept -> LogTimestampZone { return _timestampZone; }
    /// Set the timestamp rendering zone.
    /// @param value The zone used only while formatting retained UTC timestamps.
    /// @return These settings for chained configuration.
    auto setTimestampZone(LogTimestampZone value) noexcept -> LogLineFormat &;
    /// Get the severity-level representation.
    [[nodiscard]] auto levelFormat() const noexcept -> LogLevelFormat { return _levelFormat; }
    /// Set the severity-level representation.
    /// @param value The spelling and case used for rendered levels.
    /// @return These settings for chained configuration.
    auto setLevelFormat(LogLevelFormat value) noexcept -> LogLineFormat &;
    /// Get the stream-name representation.
    [[nodiscard]] auto nameFormat() const noexcept -> LogNameFormat { return _nameFormat; }
    /// Set the stream-name representation.
    /// @param value The representation used for hierarchical stream paths.
    /// @return These settings for chained configuration.
    auto setNameFormat(LogNameFormat value) noexcept -> LogLineFormat &;
    /// Get the character limit for left-truncated names.
    [[nodiscard]] auto nameLimit() const noexcept -> unit::CpLength { return _nameLimit; }
    /// Set the character limit for left-truncated names.
    /// @param value The maximum code-point count when `LogNameFormat::LeftTruncated` is selected.
    /// @return These settings for chained configuration.
    auto setNameLimit(unit::CpLength value) noexcept -> LogLineFormat &;
    /// Get the message-truncation rule.
    [[nodiscard]] auto messageTruncation() const noexcept -> LogMessageTruncation { return _messageTruncation; }
    /// Set the message-truncation rule.
    /// @param value The rendering-time truncation rule to apply.
    /// @return These settings for chained configuration.
    auto setMessageTruncation(LogMessageTruncation value) noexcept -> LogLineFormat &;
    /// Get the configured message or total-line character limit.
    [[nodiscard]] auto messageLimit() const noexcept -> unit::CpLength { return _messageLimit; }
    /// Set the message or total-line character limit.
    /// @param value The code-point limit used by the selected truncation rule.
    /// @return These settings for chained configuration.
    auto setMessageLimit(unit::CpLength value) noexcept -> LogLineFormat &;
    /// Get the mark appended to truncated text.
    [[nodiscard]] auto truncationMark() const noexcept -> const text::String & { return _truncationMark; }
    /// Set the mark appended to truncated text.
    /// @param value The marker appended after rendering-time truncation.
    /// @return These settings for chained configuration.
    auto setTruncationMark(text::String value) noexcept -> LogLineFormat &;

private:
    /// Validate braces and supported placeholders in a pattern.
    /// @param value The pattern text to validate.
    /// @return `true` if every brace and placeholder is valid.
    [[nodiscard]] static auto isValidPattern(const text::String &value) noexcept -> bool;

private:
    text::String _pattern;                                               ///< Placeholder pattern for complete lines.
    LogTimestampZone _timestampZone{LogTimestampZone::Utc};              ///< Zone used while rendering timestamps.
    LogLevelFormat _levelFormat{LogLevelFormat::ThreeLetterUpper};       ///< Rendered level spelling and case.
    LogNameFormat _nameFormat{LogNameFormat::Full};                      ///< Rendered stream-path representation.
    unit::CpLength _nameLimit{40U};                                      ///< Code-point limit for left-truncated names.
    LogMessageTruncation _messageTruncation{LogMessageTruncation::None}; ///< Rendering-time truncation rule.
    unit::CpLength _messageLimit{0U};                                    ///< Code-point limit for the selected rule.
    text::String _truncationMark;                                        ///< Marker appended to truncated text.
};

}
