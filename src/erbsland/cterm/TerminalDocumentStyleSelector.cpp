// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentStyleSelector.hpp"

#include "../text/AsciiCategory.hpp"
#include "../text/CharSet.hpp"

namespace erbsland::cterm {

using text::AsciiCategory;
using text::CharSet;
using text::String;
using text::TextNodeType;

auto TerminalDocumentStyleSelector::styleTokenSeparators() -> const CharSet & {
    static const auto cSeparators = CharSet::from(AsciiCategory::Whitespace);
    return cSeparators;
}

TerminalDocumentStyleSelector::TerminalDocumentStyleSelector(
    const TextNodeType nodeType, std::initializer_list<String> requiredStyleTokens) :
    _nodeType{nodeType}, _requiredStyleTokens{requiredStyleTokens} {
    normalizeTokens(_requiredStyleTokens);
}

TerminalDocumentStyleSelector::TerminalDocumentStyleSelector(
    TextNodeType nodeType, std::optional<int> level, std::initializer_list<String> requiredStyleTokens) :
    _nodeType{nodeType}, _level{level}, _requiredStyleTokens{requiredStyleTokens} {
    normalizeTokens(_requiredStyleTokens);
}

TerminalDocumentStyleSelector::TerminalDocumentStyleSelector(
    const TextNodeType nodeType,
    std::optional<int> level,
    std::initializer_list<String> requiredStyleTokens,
    std::optional<TextNodeType> ancestorType) :
    _nodeType{nodeType}, _level{level}, _requiredStyleTokens{requiredStyleTokens}, _ancestorType{ancestorType} {
    normalizeTokens(_requiredStyleTokens);
}

void TerminalDocumentStyleSelector::normalizeTokens(TokenList &tokens) {
    tokens.sort();
    auto previousToken = String{};
    auto hasPreviousToken = false;
    tokens.removeIf([&](const String &token) -> bool {
        if (hasPreviousToken && token == previousToken) {
            return true;
        }
        previousToken = token;
        hasPreviousToken = true;
        return false;
    });
}

auto TerminalDocumentStyleSelector::splitStyleTokens(const String &value) -> TokenList {
    auto result = TokenList::fromSplit(value, styleTokenSeparators());
    normalizeTokens(result);
    return result;
}

}
