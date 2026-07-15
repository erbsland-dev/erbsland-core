// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentStyleSelector.hpp"

#include "../text/AsciiCategory.hpp"
#include "../text/CharSet.hpp"

namespace erbsland::cterm {

auto TerminalDocumentStyleSelector::styleTokenSeparators() -> const text::CharSet & {
    static const auto cSeparators = text::CharSet::from(text::AsciiCategory::Whitespace);
    return cSeparators;
}

TerminalDocumentStyleSelector::TerminalDocumentStyleSelector(
    const text::TextNodeType nodeType, std::initializer_list<text::StringView> requiredStyleTokens) :
    _nodeType{nodeType}, _requiredStyleTokens{requiredStyleTokens} {
    normalizeTokens(_requiredStyleTokens);
}

TerminalDocumentStyleSelector::TerminalDocumentStyleSelector(
    text::TextNodeType nodeType,
    std::optional<int> level,
    std::initializer_list<text::StringView> requiredStyleTokens) :
    _nodeType{nodeType}, _level{level}, _requiredStyleTokens{requiredStyleTokens} {
    normalizeTokens(_requiredStyleTokens);
}

TerminalDocumentStyleSelector::TerminalDocumentStyleSelector(
    const text::TextNodeType nodeType,
    std::optional<int> level,
    std::initializer_list<text::StringView> requiredStyleTokens,
    std::optional<text::TextNodeType> ancestorType) :
    _nodeType{nodeType}, _level{level}, _requiredStyleTokens{requiredStyleTokens}, _ancestorType{ancestorType} {
    normalizeTokens(_requiredStyleTokens);
}

void TerminalDocumentStyleSelector::normalizeTokens(TokenList &tokens) {
    tokens.sort();
    auto previousToken = text::StringView{};
    auto hasPreviousToken = false;
    tokens.removeIf([&](const text::StringView &token) -> bool {
        if (hasPreviousToken && token == previousToken) {
            return true;
        }
        previousToken = token;
        hasPreviousToken = true;
        return false;
    });
}

auto TerminalDocumentStyleSelector::splitStyleTokens(text::StringView value) -> TokenList {
    auto result = TokenList::fromSplit(value, styleTokenSeparators());
    normalizeTokens(result);
    return result;
}

}
