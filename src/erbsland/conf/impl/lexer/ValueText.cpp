// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueText.hpp"

#include "Core.hpp"
#include "Text.hpp"
#include "ValueMultiLine.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../text/Literals.hpp"

#include <utility>

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto scanSingleLineText(TokenDecoder &decoder, const bool expandPlaceholders) -> std::optional<LexerToken> {
    if (decoder.character() != nc::doubleQuote && decoder.character() != nc::backtick &&
        decoder.character() != nc::slash) {
        return {};
    }
    auto terminatingCharacter = decoder.character();
    decoder.next();
    text::StringEditor text;
    if (terminatingCharacter == nc::doubleQuote) {
        if (expandPlaceholders && decoder.hasPlaceholders()) {
            parseTextWithPlaceholders(decoder, text);
        } else {
            parseText(decoder, text);
        }
        return decoder.createToken(TokenType::Text, std::move(text));
    }
    if (terminatingCharacter == nc::slash) {
        parseRegularExpression(decoder, text);
        return decoder.createToken(TokenType::RegEx, std::move(text));
    }
    parseCode(decoder, text);
    return decoder.createToken(TokenType::Code, std::move(text));
}

}
