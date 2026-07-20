// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueLiteral.hpp"

#include "LiteralTables.hpp"

#include "../utilities/YieldMacros.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto scanLiteral(TokenDecoder &decoder) -> std::optional<LexerToken> {
    if (decoder.character() != CharClass::Letter) {
        return std::nullopt;
    }
    auto transaction = Transaction{decoder};
    while (decoder.character() == CharClass::Letter) {
        decoder.next();
        if (transaction.capturedSize() > 8) {
            decoder.throwSyntaxError("Unknown value literal."_el);
        }
    }
    const auto identifier = transaction.capturedString().transformed(text::Char::toAsciiLowercase);
    if (identifier == "t"_el && decoder.character() == CharClass::DecimalDigit) {
        // This is most likely a time prefix - backtracking.
        return std::nullopt;
    }
    if (decoder.character() != CharClass::ValidAfterValue) {
        decoder.throwSyntaxError("Unexpected character after literal."_el);
    }
    const auto findIt = LiteralTables::identifierMap.find(identifier);
    if (findIt == LiteralTables::identifierMap.end()) {
        decoder.throwSyntaxError("Unknown value literal."_el);
    }
    transaction.commit();
    return decoder.createToken(findIt->second.type, findIt->second.value);
}

}
