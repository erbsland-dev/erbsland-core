// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::path::impl {

class PathWalker;
using PathWalkerPtr = std::unique_ptr<PathWalker>;

}
