// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AcceptedNameEnd.hpp"
#include "LexerToken.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// The result of the `expectRegularOrMetaName` function.
struct NameResult {
    bool isMetaName;
    text::String name;
};

/// Expects and reads a regular name or a metaname.
/// - Expects that the current character is the first character of the name or the @ character.
/// - Raises an error if the regular name is too long or has the wrong syntax.
/// - Handles trailing spacing.
/// - Leaves with the current character immediately *after* the name.
/// @param decoder The decoder
/// @param acceptedNameEnd What is an acceptable end of the name.
/// @return The normalized name and whether a meta-name was detected.
[[nodiscard]] auto expectRegularOrMetaName(Decoder &decoder, AcceptedNameEnd acceptedNameEnd) -> NameResult;

/// Expects and reads a regular name or a metaname.
/// @see expectRegularOrMetaName()
/// @param decoder The decoder
/// @return The token.
[[nodiscard]] auto expectRegularOrMetaNameToken(TokenDecoder &decoder) -> LexerToken;

/// Expects and reads a text name.
/// @param decoder The decoder
/// @return The token.
[[nodiscard]] auto expectTextName(TokenDecoder &decoder) -> LexerToken;

}
