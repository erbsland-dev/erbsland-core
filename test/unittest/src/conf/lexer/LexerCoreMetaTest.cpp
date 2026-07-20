// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Meta)
class LexerCoreMetaTest final : public UNITTEST_SUBCLASS(LexerTestHelper) {
public:
    void testCoreMetaDirectives() {
        setupTokenGenerator("@version: \"1.0\"\n"_el);
        WITH_CONTEXT(requireNextStringToken(TokenType::MetaName, "@version"_el, "@version"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::Text, "1.0"_el, "\"1.0\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("@features: \"regex timedelta\"\n"_el);
        WITH_CONTEXT(requireNextStringToken(TokenType::MetaName, "@features"_el, "@features"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::Text, "regex timedelta"_el, "\"regex timedelta\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("@signature: \"data\"\n"_el);
        WITH_CONTEXT(requireNextStringToken(TokenType::MetaName, "@signature"_el, "@signature"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::Text, "data"_el, "\"data\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("@parser_test: \"data\"\n"_el);
        WITH_CONTEXT(requireNextStringToken(TokenType::MetaName, "@parser_test"_el, "@parser_test"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::Text, "data"_el, "\"data\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }
};
