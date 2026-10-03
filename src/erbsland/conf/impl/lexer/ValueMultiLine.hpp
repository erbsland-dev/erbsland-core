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

/// Read the format or language and line ending after an opening delimiter.
/// @param decoder The shared decoder.
/// @param openTokenType The opening delimiter classification.
/// @return The header tokens.
/// @throws ConfError For invalid header input.
/// @tested{SyntaxParserTest LexerRecoveryTest LexerStandardMultiLineTextTest LexerStandardMultiLineBytesTest}
[[nodiscard]] auto expectMultiLineHeader(TokenDecoder &decoder, TokenType openTokenType) -> TokenGenerator;

/// Read one continued line, including indentation and an optional closing delimiter.
/// @param decoder The shared decoder.
/// @param openTokenType The opening delimiter classification.
/// @param expandPlaceholders Whether ordinary text allows placeholder expansion.
/// @param closed Set when the closing delimiter is recognized.
/// @return The physical line tokens.
/// @throws ConfError For invalid continued input.
/// @tested{SyntaxParserTest LexerRecoveryTest LexerStandardMultiLineTextTest LexerStandardMultiLineBytesTest}
[[nodiscard]] auto expectMultiLineBodyLine(
    TokenDecoder &decoder, TokenType openTokenType, bool expandPlaceholders, bool &closed) -> TokenGenerator;

/// Read an opened multiline text, code, regex or bytes value, retaining its context during syntax recovery.
/// @param decoder The shared decoder, positioned after the opening delimiter.
/// @param openTokenType The opening delimiter classification.
/// @param expandPlaceholders Whether ordinary text allows configured placeholder expansion.
/// @return The complete value tokens, including the closing line's terminator.
/// @throws ConfError For strict parsing or unrecoverable failures.
/// @tested{SyntaxParserTest LexerRecoveryTest LexerStandardMultiLineTextTest LexerStandardMultiLineBytesTest}
[[nodiscard]] auto expectMultiLineValue(TokenDecoder &decoder, TokenType openTokenType, bool expandPlaceholders)
    -> TokenGenerator;

/// Test if we reached the end of a multi-line text.
/// @param decoder The decoder.
/// @param tokenType The token type to parse.
auto isAtMultiLineEnd(const TokenDecoder &decoder, const TokenType tokenType) -> bool;

}
