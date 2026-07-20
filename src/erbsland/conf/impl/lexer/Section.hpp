// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Expects and reads a section line, with the open, names, and close tokens.
/// @param decoder The decoder
/// @return The token generator for the token sequence.
[[nodiscard]] auto expectSection(TokenDecoder &decoder) -> TokenGenerator;

}
