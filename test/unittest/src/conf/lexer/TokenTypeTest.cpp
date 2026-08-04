// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/conf/impl/lexer/TokenType.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>

using namespace erbsland::conf;
using el::conf::impl::TokenType;
namespace nc = erbsland::conf::impl::nc;

TESTED_TARGETS(TokenType)
class TokenTypeTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testDefaultAndConstructor() {
        TokenType typeDefault;
        REQUIRE_EQUAL(typeDefault, TokenType::Error);

        TokenType lineBreak{TokenType::LineBreak};
        REQUIRE_EQUAL(lineBreak, TokenType::LineBreak);
        REQUIRE_NOT_EQUAL(lineBreak, TokenType::Error);
    }

    void testMultiLineOpen() {
        const auto textOpen = TokenType::fromMultiLineOpen(nc::doubleQuote);
        const auto codeOpen = TokenType::fromMultiLineOpen(nc::backtick);
        const auto regexOpen = TokenType::fromMultiLineOpen(nc::slash);
        const auto bytesOpen = TokenType::fromMultiLineOpen(nc::lessThan);
        const auto unknownOpen = TokenType::fromMultiLineOpen(U'?');
        REQUIRE_EQUAL(textOpen, TokenType::MultiLineTextOpen);
        REQUIRE_EQUAL(codeOpen, TokenType::MultiLineCodeOpen);
        REQUIRE_EQUAL(regexOpen, TokenType::MultiLineRegexOpen);
        REQUIRE_EQUAL(bytesOpen, TokenType::MultiLineBytesOpen);
        REQUIRE_EQUAL(unknownOpen, TokenType::EndOfData);
    }

    void testMultiLineClose() {
        const auto textClose = TokenType::fromMultiLineClose(nc::doubleQuote);
        const auto codeClose = TokenType::fromMultiLineClose(nc::backtick);
        const auto regexClose = TokenType::fromMultiLineClose(nc::slash);
        const auto bytesClose = TokenType::fromMultiLineClose(nc::greaterThan);
        const auto unknownClose = TokenType::fromMultiLineClose(U'?');
        REQUIRE_EQUAL(textClose, TokenType::MultiLineTextClose);
        REQUIRE_EQUAL(codeClose, TokenType::MultiLineCodeClose);
        REQUIRE_EQUAL(regexClose, TokenType::MultiLineRegexClose);
        REQUIRE_EQUAL(bytesClose, TokenType::MultiLineBytesClose);
        REQUIRE_EQUAL(unknownClose, TokenType::EndOfData);
    }

    void testFormatter() {
        const auto boolean = std::format("{}", TokenType{TokenType::Boolean});
        const auto multiLineCodeOpen = std::format("{}", TokenType{TokenType::MultiLineCodeOpen});
        const auto error = std::format("{}", TokenType{TokenType::Error});
        REQUIRE_EQUAL(boolean, "Boolean");
        REQUIRE_EQUAL(multiLineCodeOpen, "MultiLineCodeOpen");
        REQUIRE_EQUAL(error, "Error");
    }
};
