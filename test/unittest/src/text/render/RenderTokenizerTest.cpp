// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/EnvironmentOptions.hpp>
#include <erbsland/text/render/impl/Tokenizer.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <utility>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Tokenizer Token)
class RenderTokenizerTest final : public el::UnitTest {
    using Token = el::text::render::impl::Token;
    using TokenKind = el::text::render::impl::TokenKind;
    using Tokenizer = el::text::render::impl::Tokenizer;

public:
    void testTextExpressionTokensAndLocations() {
        auto tokenizer = createTokenizer("é\n{{ user == \"x\\n\" }}tail"_el);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Text, "é\n"_el));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 0U, 0U, 0U));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 1U, 0U, 2U));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "user"_el));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 1U, 3U, 5U));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Equal));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 1U, 8U, 10U));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::String, "x\n"_el));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 1U, 11U, 13U));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 1U, 17U, 19U));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Text, "tail"_el));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 1U, 19U, 21U));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::End));
        WITH_CONTEXT(requireLocation(tokenizer.current(), 1U, 23U, 25U));
    }

    void testCommentsWhitespaceAndCustomDelimiters() {
        auto options = EnvironmentOptions{};
        options.setExpressionDelimiters(Delimiters{"[["_el, "]]"_el})
            .setStatementDelimiters(Delimiters{"<%"_el, "%>"_el})
            .setCommentDelimiters(Delimiters{"(#"_el, "#)"_el});
        auto tokenizer =
            createTokenizer(" A (#- hidden -#) B [x [[ \"inside ]] delimiter\" ]]<% set value = 7 %>"_el, options);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Text, " A"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Text, "B [x "_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::String, "inside ]] delimiter"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::StatementBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "set"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "value"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Assign));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Integer, "7"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::End));
    }

    void testQuotedClosingDelimiterAndEscapes() {
        auto tokenizer = createTokenizer("{{ 'inside }} and \\' quote and \\\\ slash' }}"_el);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::String, "inside }} and ' quote and \\ slash"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::End));
    }

    void testExpressionWhitespaceControl() {
        auto tokenizer = createTokenizer("left \t{{- value -}}\n right"_el);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Text, "left"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "value"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Text, "right"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::End));
    }

    void testClosingDelimiterPrecedesIdentifierCharacters() {
        auto options = EnvironmentOptions{};
        options.setExpressionDelimiters(Delimiters{"[["_el, "x"_el});
        auto tokenizer = createTokenizer("[[ abcxraw"_el, options);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "abc"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Text, "raw"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::End));
    }

    void testExpressionLexicalKinds() {
        auto tokenizer = createTokenizer(
            "{{ (true and false) or not null, | filter != -7 == .5 > 1.5 >= 2 < 3 <= 4 = name.member }}"_el);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::LeftParen));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::True, "true"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::And, "and"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::False, "false"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::RightParen));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Or, "or"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Not, "not"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Null, "null"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Comma));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Pipe));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "filter"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::NotEqual));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Minus));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Integer, "7"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Equal));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Float, ".5"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Greater));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Float, "1.5"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::GreaterEqual));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Integer, "2"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Less));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Integer, "3"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::LessEqual));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Integer, "4"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Assign));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "name"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Dot));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "member"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::End));
    }

    void testEmptyAndAdjacentTags() {
        auto tokenizer = createTokenizer("{{ }}{% set a = 1 %}{{a}}"_el);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::StatementBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "set"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "a"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Assign));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Integer, "1"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::ExpressionBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "a"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::End));
    }

    void testMalformedTagsAndStrings() {
        WITH_CONTEXT(requireTokenizationError("{{ value"_el, "The layout tag has no closing delimiter."_el));
        WITH_CONTEXT(requireTokenizationError("{# comment"_el, "The layout tag has no closing delimiter."_el));
        WITH_CONTEXT(requireTokenizationError("{{ \"unterminated }}"_el, "A string literal is not terminated."_el));
    }

    void testUnsupportedCharactersRemainGrammarTokens() {
        auto tokenizer = createTokenizer("{% set value ! %}"_el);

        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::StatementBegin));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "set"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Identifier, "value"_el));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::Unsupported));
        tokenizer.advance();
        WITH_CONTEXT(requireToken(tokenizer, TokenKind::TagEnd));
    }

private:
    [[nodiscard]] static auto createTokenizer(
        el::text::String source, EnvironmentOptions options = EnvironmentOptions{}) -> Tokenizer {
        return Tokenizer{"page"_el, "memory:page"_el, std::move(source), std::move(options)};
    }

    void requireToken(const Tokenizer &tokenizer, const TokenKind kind, const el::text::String &text = {}) {
        REQUIRE_EQUAL(tokenizer.current().kind, kind);
        REQUIRE_EQUAL(tokenizer.current().text, text);
    }

    void requireLocation(
        const Token &token, const std::size_t line, const std::size_t column, const std::size_t position) {
        REQUIRE_EQUAL(token.location.line(), el::unit::LineIndex::fromSizeT(line));
        REQUIRE_EQUAL(token.location.column(), el::unit::ColumnIndex::fromSizeT(column));
        REQUIRE_EQUAL(token.location.position(), el::unit::CpIndex::fromSizeT(position));
    }

    void requireTokenizationError(const el::text::String &source, const el::text::String &description) {
        try {
            auto tokenizer = createTokenizer(source);
            do {
                tokenizer.advance();
            } while (tokenizer.current().kind != TokenKind::End);
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().description(), description);
        }
    }
};
