// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// An escape function.
/// - Takes the target string as only argument.
/// - When called, the escape character is the current character.
/// - Can throw exceptions to indicate syntax errors.
/// - Must leave with the current character is the character following the escape sequence.
using EscapeFn = std::function<void(Decoder &, text::StringEditor &)>;

/// Generic function to parse a string.
/// - Expects that the current character is the first character in the string.
/// - Raises an error if the data ends, before the terminator character.
/// - Leaves with the current character *after* the terminator.
/// @param decoder The decoder to use.
/// @param target The target string where to store the characters.
/// @param terminator The terminating character for this string.
/// @param escapeChar An escape character (use zero if there is no escape character).
/// @param escapeFn The function to handle escape characters.
void parseString(
    Decoder &decoder,
    text::StringEditor &target,
    text::Char terminator,
    text::Char escapeChar,
    const EscapeFn &escapeFn);

/// Generic function to parse a multi-line string.
/// - Parses a string up to the last character, that is no trailing spacing.
/// - Throws an exception on an early end-of-data situation.
/// @param decoder The decoder to use.
/// @param escapeChar An escape character (use zero if there is no escape character).
/// @param escapeFn The function to handle escape characters.
/// @param tokenType The type of token to return.
/// @param expandPlaceholders Whether placeholders in ordinary text are expanded.
[[nodiscard]] auto parseMultiLineString(
    TokenDecoder &decoder,
    text::Char escapeChar,
    EscapeFn escapeFn,
    TokenType tokenType,
    bool expandPlaceholders = false) -> TokenGenerator;

/// Parse regular single line text.
/// @param decoder The decoder to use.
/// @param target The string where the parsed text is appended to.
/// @throws ConfError For any syntax errors in the parsed text.
void parseText(Decoder &decoder, text::StringEditor &target);

/// Parse regular single-line text and expand placeholders.
void parseTextWithPlaceholders(TokenDecoder &decoder, text::StringEditor &target);

/// Parse the escape sequence after the backslash character.
/// @param decoder The decoder to use.
/// @param target The string where the parsed text is appended to.
/// @throws ConfError For any syntax errors in the parsed text.
void parseTextEscapeSequence(Decoder &decoder, text::StringEditor &target);

/// Parse a single line regular expression text.
/// @param decoder The decoder to use.
/// @param target The string where the parsed text is appended to.
/// @throws ConfError For any syntax errors in the parsed text.
void parseRegularExpression(Decoder &decoder, text::StringEditor &target);

/// Parse the escape sequence after the backslash character.
/// @param decoder The decoder to use.
/// @param target The string where the parsed text is appended to.
/// @throws ConfError For any syntax errors in the parsed text.
void parseRegularExpressionEscapeSequence(Decoder &decoder, text::StringEditor &target);

/// Parse a single line code text.
void parseCode(Decoder &decoder, text::StringEditor &target);

}
