// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"
#include "U32StringSplitter_fwd.hpp"

#include "../impl/StringSplitter.hpp"

namespace erbsland::text {

/// A sequential UTF-32 string splitter.
using U32StringSplitter = impl::StringSplitter<U32String>;

}
