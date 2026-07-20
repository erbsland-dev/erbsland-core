// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String_fwd.hpp"

#include "../impl/StringSplitter_fwd.hpp"

namespace erbsland::text {

/// A sequential UTF-16 string splitter.
using U16StringSplitter = impl::StringSplitter<U16String>;

}
