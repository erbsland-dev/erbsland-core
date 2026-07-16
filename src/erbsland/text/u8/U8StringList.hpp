// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"
#include "U8StringList_fwd.hpp"

#include "../impl/StringList.hpp"

namespace erbsland::text {

/// A list of UTF-8 strings.
using U8StringList = impl::StringList<U8String>;

}
