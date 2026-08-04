// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IdnaDataTypes.hpp"

namespace erbsland::text::punycode::impl {

/// Look up all IDNA2008 attributes for one Unicode code point.
/// @param codePoint The Unicode code point.
/// @return The stored attributes, or disallowed attributes if the code point is absent.
/// @tested{IdnaTest}
[[nodiscard]] auto idnaAttributes(char32_t codePoint) noexcept -> IdnaAttributes;

}
