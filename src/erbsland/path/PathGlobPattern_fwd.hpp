// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::path {

class PathGlobPattern;
/// A shared, immutable, thread-safe compiled path glob.
using PathGlobPatternPtr = std::shared_ptr<const PathGlobPattern>;

}
