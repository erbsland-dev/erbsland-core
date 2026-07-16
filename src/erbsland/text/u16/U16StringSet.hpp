// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"
#include "U16StringSet_fwd.hpp"

#include "../impl/StringSet.hpp"

namespace erbsland::text {

/// A UTF-16 string-keyed ordered set.
using U16StringSet = impl::StringSet<U16String, false>;

}
