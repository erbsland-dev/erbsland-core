// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../NamePath.hpp"

#include <vector>

namespace erbsland::conf::impl {

/// Parse one name-path-like value.
[[nodiscard]] auto parseNamePathLike(const NamePathLike &namePathLike) -> NamePath;
/// Parse a list of name-path-like values.
[[nodiscard]] auto parseNamePathList(const std::vector<NamePathLike> &paths) -> NamePathList;

}
