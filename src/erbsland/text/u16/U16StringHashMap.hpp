// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"

#include "../impl/StringHashMap.hpp"

namespace erbsland::text {

/// A UTF-16 string-keyed hash map.
template <typename tValue>
using U16StringHashMap = impl::StringHashMap<U16String, tValue, false>;

}
