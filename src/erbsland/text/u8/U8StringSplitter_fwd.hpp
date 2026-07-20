// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"

#include "../impl/StringSplitter_fwd.hpp"

namespace erbsland::text {

/// A sequential UTF-8 string splitter.
using U8StringSplitter = impl::StringSplitter<U8String>;

}
