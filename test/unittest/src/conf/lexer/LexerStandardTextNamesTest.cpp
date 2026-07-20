// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

// NOTES:
// - Omitted tests for layouts that aren't valid in the language. (e.g., section with text name at the beginning).

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(TextNames)
class LexerStandardTextNamesTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testSections() {
        // focus on lexing around the text name, as most cases are already covered by the other tests.
        setupTokenGenerator("[section.\"with text\"]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::TextName, "with text"_el, "\"with text\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("[section  .   \"with text\"     ]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "  "_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "   "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::TextName, "with text"_el, "\"with text\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "     "_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testTextWithSpecialCharacters() {
        // spacing, leading and trailing whitespace, and escape sequences are all supported.
        setupTokenGenerator("[section.\"    \\t\\t    \"]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::TextName, "    \t\t    "_el, R"("    \t\t    ")"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        // more escape sequences
        setupTokenGenerator("[section.\x22😄\x5Cu0041\x5Cr\x5Cn\x5Cu{41}⇒\x5C\x22\x22]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextStringToken(
            TokenType::TextName, "😄A\r\nA⇒\""_el, "\x22😄\x5Cu0041\x5Cr\x5Cn\x5Cu{41}⇒\x5C\x22\x22"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testSectionUnexpectedEnd() {
        setupTokenGenerator("[section.\""_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::UnexpectedEnd));

        setupTokenGenerator("[section.\"  text]"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::UnexpectedEnd));
    }

    void testTextValueName() {
        setupTokenGenerator("[section]\n\"text\": 123\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::TextName, "text"_el, "\"text\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextIntegerToken(TokenType::Integer, 123, "123"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("[section]\n\"    text   \"  = 123\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::TextName, "    text   "_el, "\"    text   \""_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "  "_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, "="_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextIntegerToken(TokenType::Integer, 123, "123"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testTextValueNameUnexpectedEnd() {
        setupTokenGenerator("[section]\n\"text"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::UnexpectedEnd));
    }
};
