// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLevel.hpp"
#include "LogPath.hpp"

#include <vector>

namespace erbsland::log {

/// Level and path routing for one writer.
/// @tested{LogWriterFilterTest LogCoreTest}
class LogWriterFilter final {
public:
    /// Create a filter accepting all levels and paths.
    LogWriterFilter() = default;

public:
    /// Get the accepted severity levels.
    [[nodiscard]] auto acceptedLevels() const noexcept -> LogLevels { return _acceptedLevels; }
    /// Accept exactly the given severity levels, replacing the previous selection.
    /// @param value The complete severity mask to accept.
    /// @return This filter for chained route construction.
    auto setAcceptedLevels(LogLevels value) noexcept -> LogWriterFilter &;
    /// Accept every supported severity level except the given set.
    /// This replaces the previous severity selection.
    /// @param value The complete severity mask to reject.
    /// @return This filter for chained route construction.
    auto setRejectedLevels(LogLevels value) noexcept -> LogWriterFilter &;
    /// Get the accepted path roots.
    [[nodiscard]] auto acceptedPaths() const noexcept -> const std::vector<LogPath> & { return _acceptedPaths; }
    /// Add an accepted path root.
    /// An empty path list accepts every path. Once a root is added, matching uses complete path segments.
    /// @param value The root path to add.
    /// @return This filter for chained route construction.
    auto addAcceptedPath(LogPath value) -> LogWriterFilter &;
    /// Test whether this filter accepts the level and complete-segment path.
    /// @param level The entry severity to test.
    /// @param path The entry path to test.
    /// @return `true` if the level is enabled and the path is within an accepted root.
    [[nodiscard]] auto accepts(LogLevel level, const LogPath &path) const noexcept -> bool;

private:
    LogLevels _acceptedLevels{LogLevel::All}; ///< Accepted severity levels.
    std::vector<LogPath> _acceptedPaths;      ///< Accepted path roots, or empty to accept every path.
};

}
