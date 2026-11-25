// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::frame {

/// Repository identity supplied by the maintenance executable.
struct ToolProject {
    el::String name;          ///< Human-readable project name.
    el::String cmakeName;     ///< Exact name in the root CMake project declaration.
    el::String toolName;      ///< Command name in help and generated files.
    el::String configuration; ///< Project-relative default ELCL file.
};

}
