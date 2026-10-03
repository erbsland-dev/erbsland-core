// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathGlobData.hpp"

#include <vector>

namespace erbsland::path::impl {

/// Per-call path matching state, reused across alternatives without modifying the compiled program.
/// @tested{PathGlobPatternTest}
class PathGlobMatcher final {
public:
    /// Bind one candidate and the matching operation.
    /// @param path Nonempty candidate path, accessed component by component.
    /// @param mode Validated full or suffix matching mode.
    /// @param descendant Whether to test strict descendant feasibility.
    PathGlobMatcher(const Path &path, PathGlobMatchMode mode, bool descendant);

public: // matching
    /// Match one alternative using direct comparison or iterative state propagation.
    /// @param alternative An immutable component program.
    /// @return Whether the candidate satisfies the requested operation.
    [[nodiscard]] auto matches(const PathGlobData::Alternative &alternative) -> bool;

private:
    /// Compare a fixed number of components without allocating state buffers.
    [[nodiscard]] auto matchFixed(const PathGlobData::Alternative &alternative, bool suffix) const -> bool;
    /// Propagate all possible component positions, avoiding recursive backtracking.
    [[nodiscard]] auto matchRecursive(const PathGlobData::Alternative &alternative, bool suffix) -> bool;

private:
    const Path &_path;               ///< Borrowed candidate, valid for this call.
    PathGlobMatchMode _mode;         ///< Full or suffix matching.
    bool _descendant;                ///< Whether an additional component must remain possible.
    unit::ItemIndex _firstComponent; ///< First non-root candidate component.
};

}
