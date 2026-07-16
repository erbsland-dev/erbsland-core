// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringMap_fwd.hpp"

#include "u8/U8StringMap.hpp"

namespace erbsland::text {

/// The common UTF-8 string-keyed ordered map type.
template <typename tValue>
using StringMap = U8StringMap<tValue>;

}
