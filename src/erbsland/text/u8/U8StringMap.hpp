// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"
#include "U8StringMap_fwd.hpp"

#include "../impl/StringMap.hpp"

namespace erbsland::text {

/// A UTF-8 string-keyed ordered map.
template <typename tValue>
using U8StringMap = impl::StringMap<U8String, tValue, false>;

}
