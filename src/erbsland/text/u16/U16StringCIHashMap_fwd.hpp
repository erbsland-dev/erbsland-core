// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String_fwd.hpp"

#include "../impl/StringHashMap_fwd.hpp"

namespace erbsland::text {

/// A UTF-16 string-keyed hash map with case-insensitive key hashing and equality.
template <typename tValue>
using U16StringCIHashMap = impl::StringHashMap<U16String, tValue, true>;

}
