// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Core.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../text/AsciiCategory.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

auto expectEndOfLine(TokenDecoder &decoder, ExpectMore expectMore) -> TokenGenerator {
    if (decoder.character().isEndOfData()) {
        if (expectMore == ExpectMore::Yes) {
            decoder.throwUnexpectedEndOfDataError("Expected the data to be continued on the next line."_el);
        }
        co_return; // The line end can align with the end of the data.
    }
    // Spacing at the end of a line is allowed
    EL_YIELD_OPTIONAL(scanForSpacing(decoder));
    if (decoder.character().isEndOfData()) {
        if (expectMore == ExpectMore::Yes) {
            decoder.throwUnexpectedEndOfDataError("Expected the data to be continued on the next line."_el);
        }
        co_return; // The line end can align with the end of the data.
    }
    // After the spacing, a comment is allowed.
    if (decoder.character() == nc::commentStart) {
        EL_YIELD(expectComment(decoder));
    }
    if (decoder.character().isEndOfData()) {
        if (expectMore == ExpectMore::Yes) {
            decoder.throwUnexpectedEndOfDataError("Expected the data to be continued on the next line."_el);
        }
        co_return; // The line end can align with the end of the data.
    }
    // At this point, a line-break is expected, or the data must end.
    decoder.expect(CharClass::LineBreak, "Expected the end of the line, but got something else."_el);
    EL_YIELD(expectLinebreak(decoder));
    co_return;
}

auto expectLinebreak(TokenDecoder &decoder) -> LexerToken {
    // In the case of a CR, expect a next character that is a newline.
    if (decoder.character() == nc::carriageReturn) {
        decoder.next();
        decoder.expect(nc::newLine, "Expected a newline after a carriage return."_el);
    } else {
        decoder.expect(nc::newLine, "Expected a carriage return or newline."_el);
    }
    return decoder.createEndOfLineToken();
}

auto scanForSpacing(TokenDecoder &decoder) -> std::optional<LexerToken> {
    if (decoder.character() == CharClass::Spacing) {
        return expectSpacing(decoder);
    }
    return std::nullopt;
}

auto expectSpacing(TokenDecoder &decoder) -> LexerToken {
    decoder.expect(CharClass::Spacing, "Expected spacing, but got something else."_el);
    decoder.advanceWhile(text::AsciiCategory::Blank);
    return decoder.createToken(TokenType::Spacing);
}

auto expectAndCheckIndentation(TokenDecoder &decoder) -> LexerToken {
    decoder.expect(CharClass::Spacing, "Expected indentation, but got something else."_el);
    // If there is already a pattern set, verify it and only read the pattern characters.
    if (decoder.hasIndentationPattern()) {
        const auto pattern = decoder.indentationPattern();
        auto patternIndex = unit::ByteIndex{};
        while (patternIndex.isWithin(pattern.length())) {
            const auto patternChar = pattern.readCharAndAdvance(patternIndex);
            if (decoder.character() != patternChar.toRawValue()) {
                decoder.throwError(
                    ConfErrorCategory::Indentation,
                    "The indentation pattern on a continued line does not match the previous one."_el);
            }
            decoder.next();
        }
        return decoder.createToken(TokenType::Indentation);
    }
    // if no indentation pattern is defined, read and set one.
    decoder.advanceWhile(text::AsciiCategory::Blank);
    decoder.checkForErrorAndThrowIt();
    auto token = decoder.createToken(TokenType::Indentation);
    decoder.setIndentationPattern(token.rawText());
    return token;
}

auto expectComment(TokenDecoder &decoder) -> LexerToken {
    decoder.expectAndNext(nc::hash, "Expected end of line or a comment, but got something else."_el);
    while (decoder.character() != CharClass::LineBreakOrEnd) {
        decoder.checkForErrorAndThrowIt();
        decoder.next();
    }
    decoder.checkForErrorAndThrowIt();
    return decoder.createToken(TokenType::Comment);
}

auto scanFormatOrLanguageIdentifier(TokenDecoder &decoder, const bool throwOnLength) -> text::String {
    if (decoder.character() != CharClass::Letter) {
        return {};
    }
    text::StringEditor identifier;
    identifier.append(decoder.character().toAsciiLowercase());
    decoder.next();
    while (decoder.character() == CharClass::FormatIdentifierChar) {
        if (identifier.length().toSizeT() >= 16) { // as we only capture 7-bit chars, we can rely on the size.
            if (throwOnLength) {
                decoder.throwError(ConfErrorCategory::LimitExceeded, "Language or format identifier too long."_el);
            }
            return {};
        }
        identifier.append(decoder.character().toAsciiLowercase());
        decoder.next();
    }
    if (decoder.character().isEndOfData()) {
        decoder.throwUnexpectedEndOfDataError("Unexpected end after format or language identifier."_el);
    }
    return identifier;
}

}
