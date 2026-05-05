// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"

#include "../impl/StringHashSet.hpp"

namespace erbsland::text {

/// A UTF-16 string-keyed hash set.
using U16StringHashSet = impl::StringHashSet<U16String, false>;

}
