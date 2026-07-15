// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::path::impl {

class PathOperations;
using PathOperationsPtr = std::unique_ptr<PathOperations>;

}
