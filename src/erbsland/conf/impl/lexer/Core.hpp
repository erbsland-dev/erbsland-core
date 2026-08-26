// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// If more tokens are expected on the next line.
enum class ExpectMore : uint8_t { No, Yes };

/// Expects and reads an end-of-line sequence.
/// This function handles the following scenarios:
/// - Checks and processes spacing characters at the end of a line.
/// - Processes comments that may follow the spacing.
/// - Expects and processes a line-break or the end of the data after optional spacing and comments.
/// @param decoder The decoder
/// @param expectMore If the current line is complete and more needs to follow on the next line.
/// @return The token generator for the token sequence.
[[nodiscard]] auto expectEndOfLine(TokenDecoder &decoder, ExpectMore expectMore) -> TokenGenerator;

/// Expects and reads a line break.
/// @param decoder The decoder
/// @return The token.
[[nodiscard]] auto expectLinebreak(TokenDecoder &decoder) -> LexerToken;

/// Check if there is spacing and create a token from it.
/// @param decoder The decoder
/// @return Optional token.
[[nodiscard]] auto scanForSpacing(TokenDecoder &decoder) -> std::optional<LexerToken>;

/// Expects spacing.
/// @param decoder The decoder
/// @return The token.
[[nodiscard]] auto expectSpacing(TokenDecoder &decoder) -> LexerToken;

/// Expects and checks indentation.
/// @param decoder The decoder
/// @return The token.
[[nodiscard]] auto expectAndCheckIndentation(TokenDecoder &decoder) -> LexerToken;

/// Expect a comment.
/// @param decoder The decoder
/// @return The token.
[[nodiscard]] auto expectComment(TokenDecoder &decoder) -> LexerToken;

/// Scan for a format or language identifier.
/// Helper function scans for a sequence of letters, digits,
/// hyphens and underscores with a maximum length of 16 captured letters.
/// The sequence must start with a letter.
/// It exists to simplify reading e.g. the "hex" after "<" or "<<", or the identifier after the multi-line code
/// start token.
/// If any identifier is found, it is captured and returned, but no token is created!
/// The read position is just after the captured identifier.
/// The read stops as soon as a non-letter character is found.
/// This can be anything, even end of line or end of data.
/// No transaction is created that is rolled back on error.
/// @param decoder The decoder
/// @param throwOnLength If true, an error is thrown if the identifier is longer than 16 characters.
///     If set to false, an empty string is returned so a transaction can be rolled back.
/// @return The captured language or format string.
[[nodiscard]] auto scanFormatOrLanguageIdentifier(TokenDecoder &decoder, bool throwOnLength) -> text::String;

}
