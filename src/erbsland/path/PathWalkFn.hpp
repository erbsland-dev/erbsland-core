// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path_fwd.hpp"
#include "PathInfo_fwd.hpp"
#include "PathWalkStatus.hpp"

#include <functional>

namespace erbsland::path {

/// The path walk callback.
using PathWalkFn = std::function<PathWalkStatus(const Path &)>;

/// The path walk callback with additional path info for each path.
using PathInfoWalkFn = std::function<PathWalkStatus(const Path &, const PathInfo &)>;

}
