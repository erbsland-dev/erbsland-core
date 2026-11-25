// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/String.hpp>

namespace tool::pre_commit {

/// Repository state relevant to normalization, including contents of already-dirty files.
struct GitStateSnapshot final {
    el::String status;            ///< NUL-separated porcelain status.
    el::String unstagedDiff;      ///< Exact binary-capable worktree diff.
    el::String stagedDiff;        ///< Exact binary-capable index diff.
    el::ByteBlock untrackedFiles; ///< Digest of paths, types, and content fingerprints.

    auto operator==(const GitStateSnapshot &) const -> bool = default;
};

}
