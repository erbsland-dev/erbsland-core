// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"

#include "../impl/StringSet.hpp"

namespace erbsland::text {

/// A UTF-8 string-keyed ordered set with case-insensitive key comparison.
using U8StringCISet = impl::StringSet<U8String, true>;

}
