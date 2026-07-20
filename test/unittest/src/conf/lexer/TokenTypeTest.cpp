// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/conf/impl/lexer/TokenType.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
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
        REQUIRE(typeDefault == TokenType::Error);

        TokenType lineBreak{TokenType::LineBreak};
        REQUIRE(lineBreak == TokenType::LineBreak);
        REQUIRE(lineBreak != TokenType::Error);
    }

    void testMultiLineOpen() {
        REQUIRE(TokenType::fromMultiLineOpen(nc::doubleQuote) == TokenType::MultiLineTextOpen);
        REQUIRE(TokenType::fromMultiLineOpen(nc::backtick) == TokenType::MultiLineCodeOpen);
        REQUIRE(TokenType::fromMultiLineOpen(nc::slash) == TokenType::MultiLineRegexOpen);
        REQUIRE(TokenType::fromMultiLineOpen(nc::lessThan) == TokenType::MultiLineBytesOpen);
        REQUIRE(TokenType::fromMultiLineOpen(U'?') == TokenType::EndOfData);
    }

    void testMultiLineClose() {
        REQUIRE(TokenType::fromMultiLineClose(nc::doubleQuote) == TokenType::MultiLineTextClose);
        REQUIRE(TokenType::fromMultiLineClose(nc::backtick) == TokenType::MultiLineCodeClose);
        REQUIRE(TokenType::fromMultiLineClose(nc::slash) == TokenType::MultiLineRegexClose);
        REQUIRE(TokenType::fromMultiLineClose(nc::greaterThan) == TokenType::MultiLineBytesClose);
        REQUIRE(TokenType::fromMultiLineClose(U'?') == TokenType::EndOfData);
    }

    void testFormatter() {
        REQUIRE(std::format("{}", TokenType{TokenType::Boolean}) == "Boolean");
        REQUIRE(std::format("{}", TokenType{TokenType::MultiLineCodeOpen}) == "MultiLineCodeOpen");
        REQUIRE(std::format("{}", TokenType{TokenType::Error}) == "Error");
    }
};
