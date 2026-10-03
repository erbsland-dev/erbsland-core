// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder_fwd.hpp"

namespace erbsland::conf::impl::lexer {

/// Read one top-level statement, comment or blank line using the decoder's recovery policy.
/// @param decoder The decoder positioned at a physical line start.
/// @return All tokens of this source part, including continued lines and recovered errors.
/// @throws ConfError For strict parsing or unrecoverable failures.
/// @tested{LexerBasicTest SyntaxParserTest LexerRecoveryTest}
auto readDocumentPart(TokenDecoder &decoder) -> TokenGenerator;

}
