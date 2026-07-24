// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/lexer/LexerToken.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/time/Date.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/time/Time.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/unit/CodeLocation.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;
using impl::LexerToken;
using impl::NoContent;
using impl::TokenType;

TESTED_TARGETS(LexerToken)
class LexerTokenTest final : public el::UnitTest {
public:
    template <typename ValueType>
    void verifyToken(TokenType type, const el::text::String &rawText, const ValueType &expectedValue) {
        const el::unit::CodeLocation begin{el::unit::LineIndex::zero(), el::unit::ColumnIndex::zero()};
        const el::unit::CodeLocation end{
            el::unit::LineIndex::zero(), el::unit::ColumnIndex{rawText.characterLength().toRawValue()}};
        LexerToken token{type, begin, end, rawText, expectedValue};
        REQUIRE(token.type() == type);
        REQUIRE(token.begin() == begin);
        REQUIRE(token.end() == end);
        REQUIRE(token.rawText() == rawText);
        REQUIRE(std::holds_alternative<ValueType>(token.content()));
        REQUIRE(std::get<ValueType>(token.content()) == expectedValue);
    }

    void testNoContentToken() {
        const el::unit::CodeLocation begin{el::unit::LineIndex{2U}, el::unit::ColumnIndex::zero()};
        const el::unit::CodeLocation end{el::unit::LineIndex{2U}, el::unit::ColumnIndex{1U}};
        LexerToken token{TokenType::LineBreak, begin, end, el::text::String{"\n"_el}, NoContent{}};
        REQUIRE(token.type() == TokenType::LineBreak);
        REQUIRE(token.begin() == begin);
        REQUIRE(token.end() == end);
        REQUIRE(token.rawText() == el::text::String{"\n"_el});
        REQUIRE(std::holds_alternative<NoContent>(token.content()));
    }

    void testValueTokens() {
        verifyToken<Integer>(TokenType::Integer, "42"_el, Integer{42});
        verifyToken<bool>(TokenType::Boolean, "true"_el, true);
        verifyToken<Float>(TokenType::Float, "3.14"_el, 3.14);
        verifyToken<el::text::String>(TokenType::Text, "hello"_el, el::text::String{"hello"_el});
        verifyToken<el::time::Date>(TokenType::Date, "2024-01-02"_el, makeDate(2024, 1, 2));
        const auto time = makeTimeWithZone(12, 34, 56);
        verifyToken<el::time::TimeWithZone>(TokenType::Time, "12:34:56z"_el, time);
        el::time::DateTime dt{makeDate(2024, 1, 2), time};
        verifyToken<el::time::DateTime>(TokenType::DateTime, "2024-01-02 12:34:56z"_el, dt);
        const auto bytes = el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0x01, 0x02});
        verifyToken<el::mem::ByteBlock>(TokenType::Bytes, "<0102>"_el, bytes);
        const auto delta = el::time::CalendarDelta{el::time::Seconds{5}};
        verifyToken<el::time::CalendarDelta>(TokenType::TimeDelta, "5s"_el, delta);
    }
};
