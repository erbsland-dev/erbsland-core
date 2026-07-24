// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"

namespace erbsland::text {

/// A function that maps one decoded character to a replacement character or signal.
/// Return `Char::endOfData()` to stop transformation, or `Char::noCodePoint()` to skip the input character.
/// The function can be called more than once for the same input while the implementation sizes the result.
/// It must not rely on a particular call order or call count.
using TransformCharacterFn = Char (*)(Char character) noexcept;

}
