// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"

namespace erbsland::text {

/// A function that maps one decoded character to a replacement character or signal.
/// Return `Char::endOfData()` to stop transformation, or `Char::noCodePoint()` to skip the input character.
/// @tested{U8StringModifierTest U16StringTest U32StringTest}
using TransformCharacterFn = Char (*)(Char character) noexcept;

}
