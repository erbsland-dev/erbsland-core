// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path_fwd.hpp"

#include "../core/Definitions.hpp"

#include <functional>

namespace erbsland::path {

/// A synchronous, repeatable check before backup changes and the destination move.
using PathCommitCheckFn = std::function<void(const Path &temporary, const Path &destination)>;

}
