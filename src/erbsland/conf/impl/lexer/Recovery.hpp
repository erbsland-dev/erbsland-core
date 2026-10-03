// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"

namespace erbsland::conf::impl::lexer {

/// Consume a lexical production, recovering its first input failure in syntax mode.
/// @tested{SyntaxParserTest LexerRecoveryTest}
class Recovery final {
public:
    /// Create a recovery boundary around one production.
    /// @param decoder The shared token decoder.
    explicit Recovery(TokenDecoder &decoder) noexcept : _decoder{decoder} {}

public: // recognition
    /// Yield the production tokens and retain a failed suffix when recovery is enabled.
    /// @param source The production to consume.
    /// @return The recognized tokens, followed by an error token on a recovered failure.
    /// @throws ConfError For strict parsing or an unrecoverable failure.
    auto tokens(TokenGenerator source) -> TokenGenerator;
    /// Test whether this production recovered an input failure.
    [[nodiscard]] auto failed() const noexcept -> bool { return _failed; }

private:
    /// Consume a syntax production and capture its first recoverable failure.
    auto recoverTokens(TokenGenerator source) -> TokenGenerator;

private:
    TokenDecoder &_decoder; ///< Shared decoding and diagnostic state.
    bool _failed{};         ///< Whether the production failed.
};

}
