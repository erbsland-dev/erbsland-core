// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../system/PlatformErrorContext_fwd.hpp"
#include "../text/String.hpp"

namespace erbsland::path {

/// User-facing context for a path-domain error.
/// @tested{DiagnosticTest}
class PathErrorContext final {
public:
    /// Create a context for a failed path operation.
    /// @param title A short developer-authored title.
    /// @param description An optional developer-authored explanation.
    explicit PathErrorContext(text::String title, text::String description = {}) noexcept;

    // defaults
    PathErrorContext(const PathErrorContext &) = default;
    PathErrorContext(PathErrorContext &&) noexcept = default;
    auto operator=(const PathErrorContext &) -> PathErrorContext & = default;
    auto operator=(PathErrorContext &&) noexcept -> PathErrorContext & = default;

public: // accessors
    /// Get the operation title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Set the operation title.
    auto setTitle(text::String title) noexcept -> PathErrorContext &;
    /// Get the operation description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Set the operation description.
    auto setDescription(text::String description) noexcept -> PathErrorContext &;
    /// Get explicit help or category-derived help when available.
    [[nodiscard]] auto help() const noexcept -> text::String;
    /// Set explicit help, overriding category-derived help.
    auto setHelp(text::String help) noexcept -> PathErrorContext &;
    /// Get the source path, or an empty view if none was provided.
    [[nodiscard]] auto sourcePath() const noexcept -> const text::String & { return _sourcePath; }
    /// Set the source path.
    auto setSourcePath(text::String sourcePath) noexcept -> PathErrorContext &;
    /// Get the target path, or an empty view if none was provided.
    [[nodiscard]] auto targetPath() const noexcept -> const text::String & { return _targetPath; }
    /// Set the target path.
    auto setTargetPath(text::String targetPath) noexcept -> PathErrorContext &;
    /// Get the immutable native failure context, if available.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _platformContext;
    }
    /// Set the immutable native failure context.
    auto setPlatformContext(system::PlatformErrorContextConstPtr platformContext) noexcept -> PathErrorContext &;

private:
    [[nodiscard]] auto categoryHelp() const noexcept -> text::String;

private:
    text::String _title;
    text::String _description;
    text::String _help;
    text::String _sourcePath;
    text::String _targetPath;
    system::PlatformErrorContextConstPtr _platformContext;
};

}
