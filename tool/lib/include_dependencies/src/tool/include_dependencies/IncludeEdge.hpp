// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/LineIndex.hpp>
#include <erbsland/String.hpp>

namespace tool::include_dependencies {

/// One literal include directive, retaining its physical source location.
struct IncludeEdge {
    el::String source;     ///< Source-relative containing file.
    el::String target;     ///< Source-relative target or external include spelling.
    el::LineIndex line;    ///< Zero-based physical directive line.
    bool internal = false; ///< Quoted include resolved inside the inventory.
};

}
