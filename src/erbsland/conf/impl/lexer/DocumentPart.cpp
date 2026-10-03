// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentPart.hpp"

#include "Core.hpp"
#include "Recovery.hpp"
#include "Section.hpp"
#include "Value.hpp"

#include "../utilities/YieldMacros.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

/// Recognize one source part before applying recovery.
/// @param decoder The shared decoder.
/// @return The source part's lexical tokens.
/// @throws ConfError For invalid source input.
/// @tested{LexerBasicTest SyntaxParserTest LexerRecoveryTest}
auto expectDocumentPart(TokenDecoder &decoder) -> TokenGenerator {
    if (decoder.character() == CharClass::Spacing) {
        EL_YIELD(expectSpacing(decoder));
        if (decoder.character() == CharClass::EndOfLineStart) {
            EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
            co_return;
        }
        if (decoder.character() == CharClass::NameStart) {
            decoder.throwSyntaxError("Value names must appear at the beginning of a line without leading spaces."_el);
        }
        if (decoder.character() == CharClass::SectionStart) {
            decoder.throwSyntaxError(
                "Section declarations must start at the beginning of a line without any indentation."_el);
        }
        decoder.throwSyntaxOrUnexpectedEndError(
            "Unexpected content after indentation: only a comment or an empty line was expected at this point."_el);
    }
    if (decoder.character() == CharClass::EndOfLineStart) {
        EL_YIELD_FROM(expectEndOfLine(decoder, ExpectMore::No));
    } else if (decoder.character() == CharClass::NameStart) {
        EL_YIELD_FROM(expectNameAndValue(decoder));
    } else if (decoder.character() == CharClass::SectionStart) {
        EL_YIELD_FROM(expectSection(decoder));
    } else {
        decoder.throwSyntaxError("Expected a section, name or empty line, but got something else."_el);
    }
}

auto readDocumentPart(TokenDecoder &decoder) -> TokenGenerator {
    auto recovery = Recovery{decoder};
    for (auto &&token : recovery.tokens(expectDocumentPart(decoder))) {
        co_yield std::move(token);
    }
}

}
