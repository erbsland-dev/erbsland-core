// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"

#include "../impl/StringSet.hpp"

namespace erbsland::text {

/// A UTF-32 string-keyed ordered set.
using U32StringSet = impl::StringSet<U32String, false>;

}
