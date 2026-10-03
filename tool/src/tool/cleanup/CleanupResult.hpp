// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/ProjectTextFile.hpp"

namespace tool::cleanup {

/// One fully validated file transformation, retained until every worker succeeds.
struct CleanupResult {
    frame::ProjectTextFile file; ///< Original text and metadata snapshot.
    el::String text{};           ///< Proposed complete file text.
    el::String fingerprint{};    ///< Code fingerprint; empty for CMake files.
    bool formatted = false;      ///< Whether clang-format was actually invoked.
    bool skipped = false;        ///< Complex CMake syntax was preserved.
};

}
