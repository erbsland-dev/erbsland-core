// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Section)
class LexerStandardSectionListTest final : public UNITTEST_SUBCLASS(LexerTestHelper) {
public:
    void testSectionLists() {
        // verify a few formats.
        setupTokenGenerator("*[section]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("---*[   section]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "---*["_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "   "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("*[section   .  sub]---\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "   "_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "  "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "sub"_el, "sub"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]---"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("*[section   ]*---\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "   "_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]*---"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }
};
