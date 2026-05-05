// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"

#include "../impl/StringHashSet.hpp"

namespace erbsland::text {

/// A UTF-16 string-keyed hash set with case-insensitive key hashing and equality.
using U16StringCIHashSet = impl::StringHashSet<U16String, true>;

}
