// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Scan the character stream for multi-line strings, bytes, code or regular expressions.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanMultiLineOpen(TokenDecoder &decoder) -> std::optional<LexerToken>;

/// Scan the character stream for a closing sequence of a multi-line stream.
/// Expects the decoder to be at the indented position of a continued line.
/// @param decoder The decoder.
/// @param openTokenType The token type of the open token to match the expected characters for the closing sequence.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanMultiLineClose(TokenDecoder &decoder, TokenType openTokenType) -> std::optional<LexerToken>;

/// Expect the end of the line after an opened multi-line bracket.
/// @param decoder The decoder.
/// @return The token generator.
[[nodiscard]] auto expectMultiLineAfterOpen(TokenDecoder &decoder) -> TokenGenerator;

/// Test if we reached the end of a multi-line text.
/// @param decoder The decoder.
/// @param tokenType The token type to parse.
auto isAtMultiLineEnd(const TokenDecoder &decoder, const TokenType tokenType) -> bool;

}
