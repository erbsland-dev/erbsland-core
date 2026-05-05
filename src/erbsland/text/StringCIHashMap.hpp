// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "u8/U8StringCIHashMap.hpp"

namespace erbsland::text {

/// The common UTF-8 string-keyed hash map type with case-insensitive key hashing and equality.
template <typename tValue>
using StringCIHashMap = U8StringCIHashMap<tValue>;

}
