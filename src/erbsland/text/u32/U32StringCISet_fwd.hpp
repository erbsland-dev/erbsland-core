// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String_fwd.hpp"

#include "../impl/StringSet_fwd.hpp"

namespace erbsland::text {

/// A UTF-32 string-keyed ordered set with case-insensitive key comparison.
using U32StringCISet = impl::StringSet<U32String, true>;

}
