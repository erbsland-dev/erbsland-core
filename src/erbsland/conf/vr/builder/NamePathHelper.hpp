// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../NamePath.hpp"

#include <vector>

namespace erbsland::conf::vr::builder::detail {

[[nodiscard]] auto parseNamePathLike(const NamePathLike &namePathLike) -> NamePath;
[[nodiscard]] auto parseNamePathList(const std::vector<NamePathLike> &paths) -> NamePathList;

}
