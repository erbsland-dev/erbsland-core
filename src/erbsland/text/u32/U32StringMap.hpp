// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"

#include "../impl/StringMap.hpp"

namespace erbsland::text {

/// A UTF-32 string-keyed ordered map.
template <typename tValue>
using U32StringMap = impl::StringMap<U32String, tValue, false>;

}
