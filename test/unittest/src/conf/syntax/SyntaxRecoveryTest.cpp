// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/conf/syntax/Formatter.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(SyntaxParser)
class SyntaxRecoveryTest final : public el::UnitTest {
private:
    void requireLossless(const el::conf::syntax::DocumentPtr &document, const el::String &source) {
        auto joined = el::StringEditor{};
        auto offset = el::ByteIndex{};
        auto position = el::CodeLocation{el::LineIndex{}, el::ColumnIndex{}, el::CpIndex{}};
        for (const auto &token : document->tokens()) {
            REQUIRE_EQUAL(token->range().beginIndex(), offset);
            REQUIRE_EQUAL(source.slice(token->range()), token->text());
            REQUIRE(token->location() == position);
            auto index = el::ByteIndex{};
            while (index.isWithin(token->text().length())) {
                if (token->text().readCharAndAdvance(index) == U'\n') {
                    position.nextLine();
                } else {
                    position.nextColumn();
                }
            }
            joined.append(token->text());
            offset = token->range().endIndex();
        }
        REQUIRE_EQUAL(el::String{joined}, source);
        auto lineIndex = el::LineIndex{};
        for (const auto &line : document->lines()) {
            auto lineText = el::StringEditor{};
            for (const auto &token : document->tokensForLine(lineIndex)) {
                lineText.append(token->text());
            }
            REQUIRE_EQUAL(el::String{lineText}, source.slice(line));
            ++lineIndex;
        }
    }

    void requireRecoveredBlock(const el::String &broken, const el::conf::syntax::TokenKind resumedKind) {
        const auto source = el::join("[main]\n"_el, broken, "good: 7\n"_el);
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        REQUIRE(document->hasErrors());
        REQUIRE_EQUAL(document->diagnostics().count(), el::ItemCount{1});
        REQUIRE_EQUAL(document->statements().count(), el::ItemCount{3});
        REQUIRE(document->statements().getRef(el::ItemIndex{1}).hasErrors());
        REQUIRE_FALSE(document->statements().last().hasErrors());
        REQUIRE_EQUAL(document->statements().last().namePath()->toText(), "main.good"_el);
        auto resumed = false;
        for (const auto &token : document->tokens()) {
            if (token->kind() == resumedKind) {
                resumed = true;
            }
        }
        REQUIRE(resumed);
        WITH_CONTEXT(requireLossless(document, source));
        const auto result = el::conf::syntax::Formatter{}.format(document);
        REQUIRE(result.text().contains(broken));
        REQUIRE(result.text().contains("Good"_el));
    }

public:
    void testRecoveryInsideEveryMultilineForm() {
        using enum el::conf::syntax::TokenKind::Value;
        WITH_CONTEXT(requireRecoveredBlock("x: \"\"\"\n    bad\\q\n    valid\n    \"\"\"\n"_el, MultiLineText));
        WITH_CONTEXT(requireRecoveredBlock("x: ```\n    first\n  bad\n    valid\n    ```\n"_el, MultiLineCode));
        WITH_CONTEXT(requireRecoveredBlock("x: ///\n    first\n  bad\n    valid\n    ///\n"_el, MultiLineRegex));
        WITH_CONTEXT(requireRecoveredBlock("x: <<<hex\n    0z\n    12 ab\n    >>>\n"_el, MultiLineBytes));
        WITH_CONTEXT(requireRecoveredBlock("x:\n    * ???\n    * 42\n"_el, Integer));
        WITH_CONTEXT(requireRecoveredBlock("x:\n    * 1\n  * 2\n    * 42\n"_el, Integer));
        WITH_CONTEXT(requireRecoveredBlock("x: <<<unknown\n    12 ab\n    >>>\n"_el, MultiLineBytes));
    }

    void testMissingDelimiterLeavesNextStatement() {
        for (const auto &open : {"\"\"\""_el, "```"_el, "///"_el, "<<<"_el}) {
            const auto source = el::join("[main]\nx: "_el, open, "\n    12\nnext: 7\n"_el);
            const auto document = el::conf::syntax::Parser{}.parseText(source);
            REQUIRE_EQUAL(document->diagnostics().count(), el::ItemCount{1});
            REQUIRE_EQUAL(document->statements().count(), el::ItemCount{3});
            REQUIRE(document->statements().getRef(el::ItemIndex{1}).hasErrors());
            REQUIRE_FALSE(document->statements().last().hasErrors());
            REQUIRE_EQUAL(document->statements().last().namePath()->toText(), "main.next"_el);
            WITH_CONTEXT(requireLossless(document, source));
        }
    }

    void testFailuresAtEndOfInput() {
        for (
            const auto &source :
            {"x: \"broken\\q"_el,
                "x: \"\"\""_el,
                "x: \"\"\"\n    bad\\q"_el,
                "x: ```\n    text"_el,
                "x: ///\n    regex"_el,
                "x: <<<\n    0z"_el,
                "x:\n    *"_el,
                "x:"_el}) {
            const auto document = el::conf::syntax::Parser{}.parseText(source);
            REQUIRE_EQUAL(document->diagnostics().count(), el::ItemCount{1});
            REQUIRE(document->statements().first().hasErrors());
            REQUIRE_EQUAL(el::conf::syntax::Formatter{}.format(document).text(), source);
            WITH_CONTEXT(requireLossless(document, source));
        }
    }

    void testControlCharactersAndInvalidUtf8() {
        for (
            const auto &invalid :
            {el::unittest::th::stdStringFromHex("01"),
                el::unittest::th::stdStringFromHex("ff"),
                el::unittest::th::stdStringFromHex("e2 82")}) {
            const auto source = el::join("[main]\r\nbad: \"hé"_el, el::String{invalid}, "\"\r\ngood: 2"_el);
            const auto document = el::conf::syntax::Parser{}.parseText(source);
            REQUIRE_EQUAL(document->diagnostics().count(), el::ItemCount{1});
            REQUIRE(document->statements().getRef(el::ItemIndex{1}).hasErrors());
            REQUIRE_FALSE(document->statements().last().hasErrors());
            REQUIRE_EQUAL(document->tokensForLine(el::LineIndex{2}).first()->location().line(), el::LineIndex{2});
            WITH_CONTEXT(requireLossless(document, source));
        }
    }

    void testBlankLinesDoNotHideFollowingStatements() {
        const auto source = "  # comment\n[main]\n  \nx: 1\n"_el;
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        REQUIRE_FALSE(document->hasErrors());
        REQUIRE_EQUAL(document->statements().count(), el::ItemCount{2});
        REQUIRE_EQUAL(document->statements().last().namePath()->toText(), "main.x"_el);
        WITH_CONTEXT(requireLossless(document, source));
    }
};
