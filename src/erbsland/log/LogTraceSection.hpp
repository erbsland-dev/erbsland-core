// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"

namespace erbsland::log {

/// An optional case-sensitive trace configuration section.
/// @tested{LogCoreTest}
class LogTraceSection final {
public:
    /// Create an empty trace section.
    LogTraceSection() = default;
    /// Validate and create a trace section.
    /// @param value The case-sensitive configuration identifier, or an empty string for no section.
    /// @throws err::ParameterError If the section is not a configuration identifier.
    explicit LogTraceSection(const text::String &value);

    // defaults
    ~LogTraceSection() = default;
    LogTraceSection(const LogTraceSection &) = default;
    LogTraceSection(LogTraceSection &&) noexcept = default;
    auto operator=(const LogTraceSection &) -> LogTraceSection & = default;
    auto operator=(LogTraceSection &&) noexcept -> LogTraceSection & = default;

public:
    auto operator==(const LogTraceSection &other) const noexcept -> bool = default;
    /// Test whether this trace section is unnamed.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _value.isEmpty(); }
    /// Get the case-sensitive configuration identifier.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }
    /// Convert this section to its identifier string.
    [[nodiscard]] auto toString() const noexcept -> text::String { return _value; }

private:
    /// Validate a trace-section identifier.
    /// @param value The identifier text to validate.
    /// @return `true` if `value` is empty or a valid configuration identifier.
    [[nodiscard]] static auto isValid(const text::String &value) noexcept -> bool;

private:
    text::String _value; ///< Case-sensitive configuration identifier.
};

}
