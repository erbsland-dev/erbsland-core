// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathErrorContext.hpp"

#include "../err/RuntimeError.hpp"

#include <exception>
#include <utility>

namespace erbsland::path {

/// An error related to filesystem paths.
/// @tested{DiagnosticTest PathResolveBackendTest}
class PathError final : public err::RuntimeError {
public:
    /// Create a path error with a title only.
    explicit PathError(text::StringView title, std::exception_ptr cause = {}) noexcept;
    /// Create a path error with detailed context.
    explicit PathError(PathErrorContext context, std::exception_ptr cause = {}) noexcept;

    // defaults
    ~PathError() override = default;

public: // overrides
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the operation title.
    [[nodiscard]] auto title() const noexcept -> const text::StringView & { return _context.title(); }
    /// Get the operation description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _context.description(); }
    /// Get explicit or category-derived help.
    [[nodiscard]] auto help() const noexcept -> text::StringView { return _context.help(); }
    /// Get the source path, or an empty view.
    [[nodiscard]] auto sourcePath() const noexcept -> const text::StringView & { return _context.sourcePath(); }
    /// Get the target path, or an empty view.
    [[nodiscard]] auto targetPath() const noexcept -> const text::StringView & { return _context.targetPath(); }
    /// Get the immutable native failure context, if available.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _context.platformContext();
    }
    /// Get the complete path error context.
    [[nodiscard]] auto context() const noexcept -> const PathErrorContext & { return _context; }

private:
    PathErrorContext _context;
};

}
