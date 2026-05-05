// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringView_fwd.hpp"

#include "../impl/StringList_fwd.hpp"

namespace erbsland::text {

/// A list of UTF-8 string views.
using U8StringViewList = impl::StringList<U8StringView>;

}
