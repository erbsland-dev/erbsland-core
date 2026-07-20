// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Scan the character stream for an integer value, with an optional unit suffix.
/// The returned token can be either an integer value, but also a time-delta, depending on the suffix that is
/// following the value. If the integer could be parsed fine, but the suffix is unknown, this function
/// throws an error.
/// @param decoder The decoder.
/// @return A token if the value was found, nothing if something else was found.
[[nodiscard]] auto scanIntegerOrTimeDelta(TokenDecoder &decoder) -> std::optional<LexerToken>;

}
