// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"

#include "../impl/StringHashSet_fwd.hpp"

namespace erbsland::text {

/// A UTF-8 string-keyed hash set.
using U8StringHashSet = impl::StringHashSet<U8String, false>;

}
