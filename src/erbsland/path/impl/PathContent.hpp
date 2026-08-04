// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathContent_fwd.hpp"

#include "../Path.hpp"

namespace erbsland::path::impl {

/// Storage for a path value.
class PathContent {
public:
    /// Create empty path storage.
    PathContent() = default;
    /// Create path storage for `path`.
    explicit PathContent(Path path) noexcept;

public:
    /// Get the stored path.
    [[nodiscard]] auto path() const noexcept -> const Path &;

private:
    Path _path;
};

}
