// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Scan the character stream for a single-line bytes-value.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanBytes(TokenDecoder &decoder) -> std::optional<LexerToken>;

/// Expect and read multi-line bytes sequence
/// Expects that the opening bracket token was parsed and the decoder is now at the character
/// just after the opening bracket.
/// @param decoder The decoder.
/// @return A token generator.
[[nodiscard]] auto expectMultiLineBytes(TokenDecoder &decoder) -> TokenGenerator;

}
