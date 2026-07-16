// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"
#include "U32StringHashMap_fwd.hpp"

#include "../impl/StringHashMap.hpp"

namespace erbsland::text {

/// A UTF-32 string-keyed hash map.
template <typename tValue>
using U32StringHashMap = impl::StringHashMap<U32String, tValue, false>;

}
