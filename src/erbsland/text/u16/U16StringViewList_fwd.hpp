// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringView_fwd.hpp"

#include "../impl/StringList_fwd.hpp"

namespace erbsland::text {

/// A list of UTF-16 string views.
using U16StringViewList = impl::StringList<U16StringView>;

}
