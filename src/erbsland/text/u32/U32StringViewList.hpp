// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringView.hpp"
#include "U32StringViewList_fwd.hpp"

#include "../impl/StringList.hpp"

namespace erbsland::text {

/// A list of UTF-32 string views.
using U32StringViewList = impl::StringList<U32StringView>;

}
