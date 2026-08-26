// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenKind.hpp"

#include "../../../text/String.hpp"
#include "../../../unit/CodeLocation.hpp"

namespace erbsland::text::render::impl {

/// One lexical layout token and its source position.
/// @tested{RenderTokenizerTest}
struct Token {
    TokenKind kind{TokenKind::End}; ///< The token kind.
    String text;                    ///< Captured source text or a decoded string value.
    unit::CodeLocation location;    ///< The location of the token's first source character.

    /// Test if the token can be used as a context-specific identifier.
    [[nodiscard]] auto isIdentifier() const noexcept -> bool {
        return kind == TokenKind::Identifier || kind == TokenKind::True || kind == TokenKind::False ||
            kind == TokenKind::Null || kind == TokenKind::And || kind == TokenKind::Or || kind == TokenKind::Not;
    }
};

}
