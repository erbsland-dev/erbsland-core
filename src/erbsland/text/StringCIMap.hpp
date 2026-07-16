// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringCIMap_fwd.hpp"

#include "u8/U8StringCIMap.hpp"

namespace erbsland::text {

/// The common UTF-8 string-keyed ordered map type with case-insensitive key comparison.
template <typename tValue>
using StringCIMap = U8StringCIMap<tValue>;

}
