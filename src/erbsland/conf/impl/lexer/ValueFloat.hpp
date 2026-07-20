// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Scan the character stream for float literals like `inf` and `nan`.
[[nodiscard]] auto scanLiteralFloat(TokenDecoder &decoder) -> std::optional<LexerToken>;

/// Scan the character stream for a floating-point value.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanFloatFractionOnly(TokenDecoder &decoder) -> std::optional<LexerToken>;

/// Scan the character stream for a floating-point value.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanFloatWithWholePart(TokenDecoder &decoder) -> std::optional<LexerToken>;

}
