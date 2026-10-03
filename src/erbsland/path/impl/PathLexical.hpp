// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Path_fwd.hpp"

namespace erbsland::path::impl {

/// Normalize dot components without making a path absolute or accessing the filesystem.
/// @param path The path to normalize.
/// @return The normalized path, or empty for an empty argument.
/// @tested{PathResolveBackendTest EnvironmentChangesTest}
[[nodiscard]] auto lexicalPath(const Path &path) -> Path;

}
