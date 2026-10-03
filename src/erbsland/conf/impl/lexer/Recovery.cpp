// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Recovery.hpp"

namespace erbsland::conf::impl::lexer {

auto Recovery::tokens(TokenGenerator source) -> TokenGenerator {
    if (!_decoder.isSyntaxMode()) {
        return source;
    }
    return recoverTokens(std::move(source));
}

auto Recovery::recoverTokens(TokenGenerator source) -> TokenGenerator {
    const auto initialLine = _decoder.characterPosition().line();
    while (true) {
        auto token = std::optional<LexerToken>{};
        try {
            token = source.next();
        } catch (const ConfError &error) {
            // Destroy the failed coroutine and its transactions before advancing the stream.
            source = {};
            token = _decoder.recover(error, initialLine);
            _failed = true;
        }
        if (!token) {
            co_return;
        }
        co_yield std::move(*token);
        if (_failed) {
            co_return;
        }
    }
}

}
