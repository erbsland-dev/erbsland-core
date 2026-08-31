// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"

namespace erbsland::log {

/// A validated hierarchical log path.
/// @tested{LogCoreTest}
class LogPath final {
public:
    /// Create the root path.
    LogPath() = default;
    /// Validate and create a path.
    /// @param value The lowercase ASCII slash-delimited path, or an empty string for the root path.
    /// @throws err::ParameterError If the path does not follow the log-path syntax.
    explicit LogPath(const text::String &value);

    // defaults
    ~LogPath() = default;
    LogPath(const LogPath &) = default;
    LogPath(LogPath &&) noexcept = default;
    auto operator=(const LogPath &) -> LogPath & = default;
    auto operator=(LogPath &&) noexcept -> LogPath & = default;

public: // operators
    auto operator==(const LogPath &other) const noexcept -> bool = default;

public: // accessors
    /// Test whether this is the root path.
    [[nodiscard]] auto isRoot() const noexcept -> bool { return _value.isEmpty(); }
    /// Get the canonical slash-delimited path value.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }
    /// Convert this path to its canonical string representation.
    [[nodiscard]] auto toString() const noexcept -> text::String { return _value; }
    /// Test if this path is an equal or ancestor path of `other`.
    /// @param other The path to test against this path.
    /// @return `true` if this path contains `other` by complete path segments.
    [[nodiscard]] auto contains(const LogPath &other) const noexcept -> bool;

private:
    /// Validate the path syntax.
    /// @param value The path text to validate.
    /// @return `true` if `value` is a valid log path.
    [[nodiscard]] static auto isValid(const text::String &value) noexcept -> bool;

private:
    text::String _value; ///< Canonical slash-delimited path.
};

}
