// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"

#include "../impl/StringSet.hpp"

namespace erbsland::text {

/// A UTF-16 string-keyed ordered set with case-insensitive key comparison.
using U16StringCISet = impl::StringSet<U16String, true>;

}
