// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

namespace erbsland::mem {

/// An unsafe pointer to a character segment.
using UnsafeConstCharPtr = const char *;
/// An unsafe pointer to a character segment.
using UnsafeCharPtr = char *;
/// An unsafe pointer to a character segment.
using UnsafeConstChar8Ptr = const char8_t *;
/// An unsafe pointer to a character segment.
using UnsafeChar8Ptr = char8_t *;

}
