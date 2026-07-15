// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathOperations_fwd.hpp"

#include "../Path.hpp"
#include "../PathCopyOptions.hpp"
#include "../PathCreateDirectoryOptions.hpp"
#include "../PathCreateFileOptions.hpp"
#include "../PathMoveOptions.hpp"
#include "../PathProgress.hpp"
#include "../PathRemoveOptions.hpp"

#include <functional>
#include <utility>

namespace erbsland::path::impl {

class PathOperations {
public:
    PathOperations() = default;
    explicit PathOperations(Path path) : _path{std::move(path)} {}

public:
    [[nodiscard]] auto path() const noexcept -> const Path & { return _path; }

public:
    void removeOrThrow(PathRemoveOptions options, const PathProgressFn &progressFn);
    void copyToOrThrow(const Path &destination, PathCopyOptions options, const PathProgressFn &progressFn) const;
    void moveToOrThrow(const Path &destination, PathMoveOptions options) const;
    void createFileOrThrow(PathCreateFileOptions options) const;
    void createDirectoryOrThrow(PathCreateDirectoryOptions options) const;
    [[nodiscard]] auto setAccessProfile(PathAccessProfile profile, PathChangeOptions options) const -> bool;
    [[nodiscard]] auto addAttributes(PathAttributes attributes, PathChangeOptions options) const -> bool;
    [[nodiscard]] auto clearAttributes(PathAttributes attributes, PathChangeOptions options) const -> bool;

private:
    [[nodiscard]] auto countForProgress(SymlinkMode symlinkMode) const -> unit::ElementCount;
    static void createParentsOrThrow(const Path &path);
    static void removeExistingOrThrow(const Path &path);
    static void reportProgress(
        const PathProgressFn &progressFn,
        PathProgressStatus status,
        unit::ElementCount total,
        unit::ElementCount processed,
        unit::ElementCount errors);
    [[nodiscard]] auto applyChange(
        const PathChangeOptions &options, const std::function<void(const Path &)> &changeFn) const -> bool;

private:
    Path _path;
};

}
