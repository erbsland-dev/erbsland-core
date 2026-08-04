// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/impl/diagnostics/AssemblerToken.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::text::literals;
using impl::AssemblerToken;
using impl::Operation;
using impl::OperationModifier;

TESTED_TARGETS(AssemblerToken)
TAGS(Diagnostics)
class AssemblerTokenTest final : public el::UnitTest {
public:
    void testAccessorsAndPredicates() {
        const AssemblerToken integerToken{AssemblerToken::Integer, 123U, el::unit::ColumnIndex{7U}};
        REQUIRE_EQUAL(integerToken.type(), AssemblerToken::Integer);
        REQUIRE_EQUAL(integerToken.getInteger(), 123U);
        REQUIRE_EQUAL(integerToken.column(), el::unit::ColumnIndex{7U});
        REQUIRE(integerToken.isInteger());
        REQUIRE(integerToken.isArgument());
        REQUIRE_FALSE(integerToken.isText());
        REQUIRE_FALSE(integerToken.isOperation());

        const AssemblerToken textToken{AssemblerToken::Text, "hello"_el, el::unit::ColumnIndex{1U}};
        REQUIRE(textToken.isText());
        REQUIRE(textToken.isArgument());
        REQUIRE_EQUAL(textToken.getText(), "hello"_el);

        const AssemblerToken charToken{AssemblerToken::Char, uint32_t{U'Z'}, el::unit::ColumnIndex{2U}};
        REQUIRE(charToken.isChar());
        REQUIRE(charToken.isArgument());
        REQUIRE_EQUAL(charToken.getInteger(), static_cast<uint32_t>(U'Z'));

        const AssemblerToken booleanToken{AssemblerToken::Boolean, true, el::unit::ColumnIndex{3U}};
        REQUIRE(booleanToken.isBoolean());
        REQUIRE(booleanToken.isArgument());
        REQUIRE_EQUAL(booleanToken.getBoolean(), true);

        const AssemblerToken modifierToken{
            AssemblerToken::Modifier, OperationModifier::CaseInsensitive, el::unit::ColumnIndex{4U}};
        REQUIRE(modifierToken.isModifier());
        REQUIRE_FALSE(modifierToken.isArgument());
        REQUIRE_EQUAL(modifierToken.getModifier(), OperationModifier::CaseInsensitive);

        const AssemblerToken operationToken{
            AssemblerToken::Operation, Operation{Operation::Match}, el::unit::ColumnIndex{5U}};
        REQUIRE(operationToken.isOperation());
        REQUIRE_FALSE(operationToken.isArgument());
        REQUIRE_EQUAL(operationToken.getOperation(), Operation::Match);

        const AssemblerToken labelToken{AssemblerToken::Label, "label"_el, el::unit::ColumnIndex{6U}};
        REQUIRE(labelToken.isLabel());
        REQUIRE(labelToken.isArgument());
        REQUIRE_EQUAL(labelToken.getText(), "label"_el);

        const AssemblerToken identifierToken{AssemblerToken::Identifier, "Id_1"_el, el::unit::ColumnIndex{7U}};
        REQUIRE(identifierToken.isIdentifier());
        REQUIRE(identifierToken.isArgument());
        REQUIRE_EQUAL(identifierToken.getText(), "Id_1"_el);

        const AssemblerToken offsetToken{AssemblerToken::Offset, 0x12abU, el::unit::ColumnIndex{8U}};
        REQUIRE(offsetToken.isOffset());
        REQUIRE(offsetToken.isArgument());
        REQUIRE_EQUAL(offsetToken.getInteger(), 0x12abU);

        const AssemblerToken commaToken{AssemblerToken::Comma, 0U, el::unit::ColumnIndex{9U}};
        REQUIRE(commaToken.isComma());
        REQUIRE_FALSE(commaToken.isArgument());

        const AssemblerToken minusToken{AssemblerToken::Minus, 0U, el::unit::ColumnIndex{10U}};
        REQUIRE(minusToken.isMinus());
        REQUIRE_FALSE(minusToken.isArgument());

        const AssemblerToken commandToken{AssemblerToken::Command, "data"_el, el::unit::ColumnIndex{11U}};
        REQUIRE(commandToken.isCommand());
        REQUIRE_FALSE(commandToken.isArgument());

        const AssemblerToken commentToken{
            AssemblerToken::Comment, el::text::StringEditor{}, el::unit::ColumnIndex{12U}};
        REQUIRE_EQUAL(commentToken.type(), AssemblerToken::Comment);
        REQUIRE_FALSE(commentToken.isArgument());
    }

    void testToString() {
        const AssemblerToken token{AssemblerToken::Integer, 123U, el::unit::ColumnIndex{7U}};
        REQUIRE_EQUAL(token.toString(), "col=7 type=Integer value=123"_el);
    }

    void testFormat() {
        const AssemblerToken token{AssemblerToken::Integer, 123U, el::unit::ColumnIndex{7U}};
        const auto formatted = std::format("{}", token);
        REQUIRE_EQUAL(formatted, "col=7 type=Integer value=123");
    }
};
