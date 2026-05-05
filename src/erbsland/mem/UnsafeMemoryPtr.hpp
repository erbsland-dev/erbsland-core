// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

namespace erbsland::mem {

/// Marks an unsafe pointer to a memory segment.
/// @note If you see one of those, ... run!
using UnsafeConstMemoryPtr = const void *;
/// Marks an unsafe pointer to a memory segment.
/// @note If you see one of those, ... run!
using UnsafeMemoryPtr = void *;

}
