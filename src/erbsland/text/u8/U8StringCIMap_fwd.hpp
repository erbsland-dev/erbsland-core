// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"

#include "../impl/StringMap_fwd.hpp"

namespace erbsland::text {

/// A UTF-8 string-keyed ordered map with case-insensitive key comparison.
template <typename tValue>
using U8StringCIMap = impl::StringMap<U8String, tValue, true>;

}
