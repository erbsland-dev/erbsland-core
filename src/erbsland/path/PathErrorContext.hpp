// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../system/PlatformErrorContext_fwd.hpp"
#include "../text/StringView.hpp"

namespace erbsland::path {

/// User-facing context for a path-domain error.
/// @tested{DiagnosticTest}
class PathErrorContext final {
public:
    /// Create a context for a failed path operation.
    /// @param title A short developer-authored title.
    /// @param description An optional developer-authored explanation.
    explicit PathErrorContext(text::StringView title, text::StringView description = {}) noexcept;

    // defaults
    PathErrorContext(const PathErrorContext &) = default;
    PathErrorContext(PathErrorContext &&) noexcept = default;
    auto operator=(const PathErrorContext &) -> PathErrorContext & = default;
    auto operator=(PathErrorContext &&) noexcept -> PathErrorContext & = default;

public: // accessors
    /// Get the operation title.
    [[nodiscard]] auto title() const noexcept -> const text::StringView & { return _title; }
    /// Set the operation title.
    auto setTitle(text::StringView title) noexcept -> PathErrorContext &;
    /// Get the operation description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _description; }
    /// Set the operation description.
    auto setDescription(text::StringView description) noexcept -> PathErrorContext &;
    /// Get explicit help or category-derived help when available.
    [[nodiscard]] auto help() const noexcept -> text::StringView;
    /// Set explicit help, overriding category-derived help.
    auto setHelp(text::StringView help) noexcept -> PathErrorContext &;
    /// Get the source path, or an empty view if none was provided.
    [[nodiscard]] auto sourcePath() const noexcept -> const text::StringView & { return _sourcePath; }
    /// Set the source path.
    auto setSourcePath(text::StringView sourcePath) noexcept -> PathErrorContext &;
    /// Get the target path, or an empty view if none was provided.
    [[nodiscard]] auto targetPath() const noexcept -> const text::StringView & { return _targetPath; }
    /// Set the target path.
    auto setTargetPath(text::StringView targetPath) noexcept -> PathErrorContext &;
    /// Get the immutable native failure context, if available.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _platformContext;
    }
    /// Set the immutable native failure context.
    auto setPlatformContext(system::PlatformErrorContextConstPtr platformContext) noexcept -> PathErrorContext &;

private:
    [[nodiscard]] auto categoryHelp() const noexcept -> text::StringView;

private:
    text::StringView _title;
    text::StringView _description;
    text::StringView _help;
    text::StringView _sourcePath;
    text::StringView _targetPath;
    system::PlatformErrorContextConstPtr _platformContext;
};

}
