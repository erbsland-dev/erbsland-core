// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>

namespace erbsland::re {

/// A position in the input stream.
/// This is a valid position in the input.
/// The concrete meaning depends on the used `Input` implementation.
/// - UTF8 => the position of the start byte (char, char8_t) for a character.
/// - UTF16 => the position of the start word (char16_t) for a character.
/// - UTF32 => the position of the character (char32_t).
using InputPosition = std::size_t;

}
