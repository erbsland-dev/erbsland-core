// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"

#include "../impl/StringHashSet.hpp"

namespace erbsland::text {

/// A UTF-8 string-keyed hash set with case-insensitive key hashing and equality.
using U8StringCIHashSet = impl::StringHashSet<U8String, true>;

}
