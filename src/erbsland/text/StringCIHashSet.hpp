// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringCIHashSet_fwd.hpp"

#include "u8/U8StringCIHashSet.hpp"

namespace erbsland::text {

/// The common UTF-8 string-keyed hash set type with case-insensitive key hashing and equality.
using StringCIHashSet = U8StringCIHashSet;

}
