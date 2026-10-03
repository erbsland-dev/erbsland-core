// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::include_dependencies {

/// One source-root-relative library translation unit or header.
struct IncludeNode {
    el::String path;       ///< Source-relative POSIX path.
    el::String role;       ///< Header, forward, template, source, or generated.
    bool isPublic = false; ///< Whether the publication map contains this regular header.
};

}
