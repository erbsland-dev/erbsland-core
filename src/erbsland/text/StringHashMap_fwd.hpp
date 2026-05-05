// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "u8/U8StringHashMap_fwd.hpp"

namespace erbsland::text {

/// The common UTF-8 string-keyed hash map type.
template <typename tValue>
using StringHashMap = U8StringHashMap<tValue>;

}
