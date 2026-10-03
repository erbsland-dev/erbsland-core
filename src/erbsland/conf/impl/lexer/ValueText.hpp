// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Scan the character stream for a single line text/code/regex value.
/// @param decoder The decoder.
/// @param expandPlaceholders Whether placeholders in ordinary text are expanded.
/// @return A `Text`, `Code` or `re::RegExPtr` token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanSingleLineText(TokenDecoder &decoder, bool expandPlaceholders = true)
    -> std::optional<LexerToken>;

}
