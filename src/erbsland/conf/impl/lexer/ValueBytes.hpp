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

/// Parse one line of an already opened multiline bytes value.
/// @param decoder Active token decoder.
/// @return Tokens for this content line.
/// @tested{SyntaxParserTest LexerStandardMultiLineBytesTest}
auto parseMultiLineBytesHexLine(TokenDecoder &decoder) -> TokenGenerator;

}
