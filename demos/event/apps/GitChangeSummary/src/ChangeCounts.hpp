// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace demo {

/// Counts of changed entries in one repository-relative directory.
struct ChangeCounts {
    std::uint64_t staged{};     ///< Entries with index changes, excluding conflicts.
    std::uint64_t unstaged{};   ///< Entries with working-tree changes, excluding conflicts.
    std::uint64_t untracked{};  ///< Individual untracked files.
    std::uint64_t conflicted{}; ///< Unmerged entries, counted only here.
};

}
