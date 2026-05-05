// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String_fwd.hpp"

#include "../impl/StringHashSet_fwd.hpp"

namespace erbsland::text {

/// A UTF-32 string-keyed hash set with case-insensitive key hashing and equality.
using U32StringCIHashSet = impl::StringHashSet<U32String, true>;

}
