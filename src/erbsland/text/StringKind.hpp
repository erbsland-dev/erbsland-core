// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "String_fwd.hpp"

#include <cstdint>

namespace erbsland::text {

/// The string encoding kind used by generic text APIs.
/// The indexes of this enum are used as type indices for std::variant.
enum class StringKind : uint8_t { U8 = 0, U16 = 1, U32 = 2 };

/// Convert a string kind to its descriptive text.
auto toString(StringKind kind) -> String;

}
