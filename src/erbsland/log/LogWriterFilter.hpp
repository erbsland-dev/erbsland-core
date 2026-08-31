// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLevel.hpp"
#include "LogPath.hpp"

#include <vector>

namespace erbsland::log {

/// Level and path routing for one writer.
/// @tested{LogCoreTest}
class LogWriterFilter final {
public:
    /// Create a filter accepting all levels and paths.
    LogWriterFilter() = default;
    /// Create a filter accepting the given levels and all paths.
    /// @param levels The set of severity levels to accept.
    explicit LogWriterFilter(LogLevels levels) noexcept : _levels{levels} {}

public:
    /// Get the accepted severity levels.
    [[nodiscard]] auto levels() const noexcept -> LogLevels { return _levels; }
    /// Set the accepted severity levels.
    /// @param value The complete severity mask to accept.
    /// @return This filter for chained route construction.
    auto setLevels(LogLevels value) noexcept -> LogWriterFilter &;
    /// Get the accepted path roots.
    [[nodiscard]] auto paths() const noexcept -> const std::vector<LogPath> & { return _paths; }
    /// Add an accepted path root.
    /// An empty path list accepts every path. Once a root is added, matching uses complete path segments.
    /// @param value The root path to add.
    /// @return This filter for chained route construction.
    auto addPath(LogPath value) -> LogWriterFilter &;
    /// Test whether this filter accepts the level and complete-segment path.
    /// @param level The entry severity to test.
    /// @param path The entry path to test.
    /// @return `true` if the level is enabled and the path is within an accepted root.
    [[nodiscard]] auto accepts(LogLevel level, const LogPath &path) const noexcept -> bool;

private:
    LogLevels _levels{LogLevel::All}; ///< Accepted severity levels.
    std::vector<LogPath> _paths;      ///< Accepted path roots, or empty to accept every path.
};

}
