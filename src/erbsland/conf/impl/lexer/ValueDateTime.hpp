// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Scan the character stream for a date or date time value.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanDateOrDateTime(TokenDecoder &decoder) -> std::optional<LexerToken>;

/// Scan the character stream for a time value.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanTime(TokenDecoder &decoder) -> std::optional<LexerToken>;

}
