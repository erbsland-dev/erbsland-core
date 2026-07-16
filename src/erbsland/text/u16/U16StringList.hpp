// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"
#include "U16StringList_fwd.hpp"

#include "../impl/StringList.hpp"

namespace erbsland::text {

/// A list of UTF-16 strings.
using U16StringList = impl::StringList<U16String>;

}
