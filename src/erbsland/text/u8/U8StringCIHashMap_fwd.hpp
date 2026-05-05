// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"

#include "../impl/StringHashMap_fwd.hpp"

namespace erbsland::text {

/// A UTF-8 string-keyed hash map with case-insensitive key hashing and equality.
template <typename tValue>
using U8StringCIHashMap = impl::StringHashMap<U8String, tValue, true>;

}
