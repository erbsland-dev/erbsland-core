// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"

#include "../impl/StringHashMap.hpp"

namespace erbsland::text {

/// A UTF-8 string-keyed hash map.
template <typename tValue>
using U8StringHashMap = impl::StringHashMap<U8String, tValue, false>;

}
