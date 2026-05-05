// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"

#include "../impl/StringMap.hpp"

namespace erbsland::text {

/// A UTF-16 string-keyed ordered map with case-insensitive key comparison.
template <typename tValue>
using U16StringCIMap = impl::StringMap<U16String, tValue, true>;

}
