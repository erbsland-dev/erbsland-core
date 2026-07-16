// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"
#include "U32StringHashSet_fwd.hpp"

#include "../impl/StringHashSet.hpp"

namespace erbsland::text {

/// A UTF-32 string-keyed hash set.
using U32StringHashSet = impl::StringHashSet<U32String, false>;

}
