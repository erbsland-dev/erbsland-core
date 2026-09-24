// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MultiLineAllowed.hpp"
#include "NextLine.hpp"
#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Expect and read a "name: value" sequence in the document.
/// @param decoder The decoder.
/// @return The token generator.
[[nodiscard]] auto expectNameAndValue(TokenDecoder &decoder) -> TokenGenerator;

/// Expect and read a value or value list.
/// @param decoder The decoder.
/// @param nextLine If the value is starting on the next line.
/// @param multiLineAllowed If multi-line values are allowed at this location.
/// @param expandPlaceholders Whether placeholders in ordinary text are expanded.
/// @return The token generator.
[[nodiscard]] auto expectValueOrValueList(
    TokenDecoder &decoder, NextLine nextLine, MultiLineAllowed multiLineAllowed, bool expandPlaceholders = true)
    -> TokenGenerator;

/// Expect and read a single-line value or single-line value-list.
/// @param decoder The decoder.
/// @param expandPlaceholders Whether placeholders in ordinary text are expanded.
/// @return The token generator.
[[nodiscard]] auto expectSingleLineValueOrValueList(TokenDecoder &decoder, bool expandPlaceholders = true)
    -> TokenGenerator;

/// Expect and read a single line value.
/// @param decoder The decoder.
/// @param expandPlaceholders Whether placeholders in ordinary text are expanded.
/// @return The token generator.
[[nodiscard]] auto expectSingleLineValue(TokenDecoder &decoder, bool expandPlaceholders = true) -> LexerToken;

}
