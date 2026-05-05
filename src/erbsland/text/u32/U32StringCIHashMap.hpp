// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"

#include "../impl/StringHashMap.hpp"

namespace erbsland::text {

/// A UTF-32 string-keyed hash map with case-insensitive key hashing and equality.
template <typename tValue>
using U32StringCIHashMap = impl::StringHashMap<U32String, tValue, true>;

}
