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

class PathWalker {
    enum class VisitResult {
        Continue,
        Skip,
        Stop,
        Failure,
    };

    using WalkFn = std::function<PathWalkStatus(const Path &, const PathInfo &)>;

public:
    PathWalker() = default;
    explicit PathWalker(Path path) : _path{std::move(path)} {}

public: // accessors
    [[nodiscard]] auto path() const noexcept -> const Path & { return _path; }

public:
    [[nodiscard]] auto walkOrThrow(const PathWalkFn &walkFn, PathWalkOptions options) const -> PathWalkResult;
    [[nodiscard]] auto walkOrThrow(const PathInfoWalkFn &walkFn, PathWalkOptions options) const -> PathWalkResult;

private:
    [[nodiscard]] auto walkImpl(const WalkFn &walkFn, const PathWalkOptions &options) const -> PathWalkResult;
    [[nodiscard]] auto visit(
        const Path &path,
        const WalkFn &walkFn,
        const PathWalkOptions &options,
        text::StringSet &visitedDirectories,
        bool &hadErrors,
        bool trustCache,
        const PathInfoCacheTrustPtr &cacheTrust) const -> VisitResult;
    [[nodiscard]] static auto callbackResult(PathWalkStatus status) noexcept -> VisitResult;

private:
    Path _path;
};

}
