// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathWalker_fwd.hpp"

#include "../Path.hpp"
#include "../PathInfo.hpp"
#include "../PathWalkFn.hpp"
#include "../PathWalkOptions.hpp"
#include "../PathWalkResult.hpp"

#include "../../text/StringSet.hpp"

#include <functional>
#include <memory>
#include <utility>

namespace erbsland::path::impl {

/// Traverses a path hierarchy according to path-walk options.
class PathWalker {
    /// Describes the outcome of visiting one path.
    enum class VisitResult {
        Continue,
        Skip,
        Stop,
        Failure,
    };

    using WalkFn = std::function<PathWalkStatus(const Path &, const PathInfo &)>;

public:
    /// Create a walker without a starting path.
    PathWalker() = default;
    /// Create a walker for a starting path.
    explicit PathWalker(Path path) : _path{std::move(path)} {}

public: // accessors
    /// Get the configured starting path.
    [[nodiscard]] auto path() const noexcept -> const Path & { return _path; }

public:
    /// Walk paths using a path-only callback.
    [[nodiscard]] auto walkOrThrow(const PathWalkFn &walkFn, PathWalkOptions options) const -> PathWalkResult;
    /// Walk paths using a callback with path information.
    [[nodiscard]] auto walkOrThrow(const PathInfoWalkFn &walkFn, PathWalkOptions options) const -> PathWalkResult;

private:
    /// Walk paths through the normalized internal callback.
    [[nodiscard]] auto walkImpl(const WalkFn &walkFn, const PathWalkOptions &options) const -> PathWalkResult;
    /// Visit a path and, when applicable, its child paths.
    [[nodiscard]] auto visit(
        const Path &path,
        const WalkFn &walkFn,
        const PathWalkOptions &options,
        text::StringSet &visitedDirectories,
        bool &hadErrors,
        bool trustCache,
        const PathInfoCacheTrustPtr &cacheTrust) const -> VisitResult;
    /// Convert a callback status to an internal visit result.
    [[nodiscard]] static auto callbackResult(PathWalkStatus status) noexcept -> VisitResult;

private:
    Path _path;
};

}
