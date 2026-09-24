// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZipErrorReason.hpp"
#include "ZipOperationPhase.hpp"

#include "../../path/Path.hpp"
#include "../../text/String.hpp"

#include <utility>

namespace erbsland::compression::zip {

/// User-facing context for a ZIP archive failure.
/// @tested{ZipArchiveTest}
class ZipErrorContext final {
public:
    /// Create ZIP failure context with machine-readable and user-facing fields.
    explicit ZipErrorContext(
        ZipErrorReason reason, ZipOperationPhase phase, text::String title, text::String description = {}) noexcept :
        _reason{reason}, _phase{phase}, _title{std::move(title)}, _description{std::move(description)} {}

public:
    /// Get the machine-readable failure reason.
    [[nodiscard]] auto reason() const noexcept -> ZipErrorReason { return _reason; }
    /// Get the operation phase that failed.
    [[nodiscard]] auto phase() const noexcept -> ZipOperationPhase { return _phase; }
    /// Get the user-facing summary of what failed.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Get the user-facing explanation of why it failed.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Get the optional archive or input source path.
    [[nodiscard]] auto sourcePath() const noexcept -> const path::Path & { return _sourcePath; }
    /// Get the optional archive or extraction destination path.
    [[nodiscard]] auto destinationPath() const noexcept -> const path::Path & { return _destinationPath; }
    /// Get the optional normalized archive item path.
    [[nodiscard]] auto itemPath() const noexcept -> const path::Path & { return _itemPath; }
    /// Set the archive or input source path.
    auto setSourcePath(path::Path value) noexcept -> ZipErrorContext & {
        _sourcePath = std::move(value);
        return *this;
    }
    /// Set the archive or extraction destination path.
    auto setDestinationPath(path::Path value) noexcept -> ZipErrorContext & {
        _destinationPath = std::move(value);
        return *this;
    }
    /// Set the normalized archive item path.
    auto setItemPath(path::Path value) noexcept -> ZipErrorContext & {
        _itemPath = std::move(value);
        return *this;
    }

private:
    ZipErrorReason _reason;
    ZipOperationPhase _phase;
    text::String _title;
    text::String _description;
    path::Path _sourcePath;
    path::Path _destinationPath;
    path::Path _itemPath;
};

}
