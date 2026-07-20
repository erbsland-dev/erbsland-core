// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Scan the character stream for a literal value, like booleans, NaN or Inf.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanLiteral(TokenDecoder &decoder) -> std::optional<LexerToken>;

}
