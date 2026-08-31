// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogFileMode.hpp"
#include "LogFileRotation.hpp"

#include "../path/Path.hpp"
#include "../unit/ByteLength.hpp"

#include <cstddef>
#include <utility>

namespace erbsland::log {

/// Settings for a resilient file log writer.
/// @tested{LogWriterTest}
class FileLogWriterOptions final {
public:
    /// Create options for the given base path.
    /// @param path The active log-file path from which archive names are derived.
    explicit FileLogWriterOptions(path::Path path) : _path{std::move(path)} {}

    /// Get the configured base path.
    [[nodiscard]] auto path() const noexcept -> const path::Path & { return _path; }
    /// Get the initial open mode.
    [[nodiscard]] auto mode() const noexcept -> LogFileMode { return _mode; }
    /// Set the initial open mode.
    /// @param value Whether the first open preserves or replaces existing content.
    /// @return These options for chained configuration.
    auto setMode(LogFileMode value) noexcept -> FileLogWriterOptions &;
    /// Get the rotation rule.
    [[nodiscard]] auto rotation() const noexcept -> LogFileRotation { return _rotation; }
    /// Set the rotation rule.
    /// @param value The schedule or size rule selecting rotation.
    /// @return These options for chained configuration.
    auto setRotation(LogFileRotation value) noexcept -> FileLogWriterOptions &;
    /// Get the file-size rotation threshold.
    [[nodiscard]] auto maximumSize() const noexcept -> unit::ByteLength { return _maximumSize; }
    /// Set the file-size rotation threshold.
    /// @param value The maximum active-file size before rotation.
    /// @return These options for chained configuration.
    auto setMaximumSize(unit::ByteLength value) noexcept -> FileLogWriterOptions &;
    /// Get the maximum number of retained archives.
    [[nodiscard]] auto retention() const noexcept -> std::size_t { return _retention; }
    /// Set the maximum number of retained archives.
    /// @param value The archive count to retain; zero removes every rotated archive.
    /// @return These options for chained configuration.
    auto setRetention(std::size_t value) noexcept -> FileLogWriterOptions &;

private:
    path::Path _path;                                   ///< Active file and archive-name base path.
    LogFileMode _mode{LogFileMode::Append};             ///< Mode used for the first successful open.
    LogFileRotation _rotation{LogFileRotation::None};   ///< Rotation schedule or size rule.
    unit::ByteLength _maximumSize{10U * 1024U * 1024U}; ///< Size threshold for size rotation.
    std::size_t _retention{7U};                         ///< Maximum retained archive count.
};

}
