// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/conf/syntax/Formatter.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
TESTED_TARGETS(SyntaxParser)
class SyntaxParserTest final : public el::UnitTest {
public:
    void testSharedTokensAndStoredLineLists() {
        auto document = el::conf::syntax::Parser{}.parseText("[main]\r\nx: 1"_el);
        REQUIRE(document);
        const auto &line = document->tokensForLine(el::LineIndex{});
        REQUIRE(&line == &document->tokensForLine(el::LineIndex{}));
        REQUIRE(line.first() == document->tokens().first());
        REQUIRE(document->tokensForLine(el::LineIndex::noIndex()).isEmpty());
        REQUIRE(&document->tokensForLine(el::LineIndex::noIndex()) == &document->tokensForLine(el::LineIndex{100}));
        const auto token = line.first();
        const auto retainedLine = line;
        document.reset();
        REQUIRE_EQUAL(token->text(), "["_el);
        REQUIRE(retainedLine.first() == token);
        REQUIRE_EQUAL(retainedLine.last()->text(), "\r\n"_el);
        REQUIRE(el::conf::syntax::Parser{}.parseText({}));
    }

    void testLosslessTokensAndLocations() {
        const el::String source = "# α\r\n[my_section]\r\ntext = \"héllo\" # comment\r\nlast: 3"_el;
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        REQUIRE_FALSE(document->hasErrors());
        auto joined = el::StringEditor{};
        auto offset = el::ByteIndex{};
        for (const auto &token : document->tokens()) {
            REQUIRE_EQUAL(token->range().beginIndex(), offset);
            REQUIRE_EQUAL(source.slice(token->range()), token->text());
            joined.append(token->text());
            offset = token->range().endIndex();
        }
        REQUIRE_EQUAL(el::String{joined}, source);
        REQUIRE_EQUAL(document->statements().count().toSizeT(), 3U);
        REQUIRE_EQUAL(document->lines().count().toSizeT(), 4U);
        REQUIRE_EQUAL(document->statements().getRef(el::ItemIndex{1}).namePath()->toText(), "my_section.text"_el);
        REQUIRE_FALSE(document->tokensForLine(el::LineIndex{2}).isEmpty());
        REQUIRE(document->tokensForLine(el::LineIndex{20}).isEmpty());
    }
    void testLineRecoveryPreservesMalformedAssignment() {
        const el::String source = "[main]\nbad: \"broken\\q\"\ngood:2\n"_el;
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        REQUIRE(document->hasErrors());
        REQUIRE_EQUAL(document->statements().count().toSizeT(), 3U);
        REQUIRE(document->statements().getRef(el::ItemIndex{1}).hasErrors());
        REQUIRE_FALSE(document->statements().getRef(el::ItemIndex{2}).hasErrors());
        const auto output = el::conf::syntax::Formatter{}.format(document).text();
        REQUIRE(output.contains("bad: \"broken\\q\"\n"_el));
        REQUIRE(output.contains("Good"_el));
        REQUIRE_EQUAL(document->statements().getRef(el::ItemIndex{2}).namePath()->toText(), "main.good"_el);
    }
    void testMultilineRecoveryKeepsContext() {
        const el::String source = "[main]\ntext: \"\"\"\n    bad\\q\n    valid text\n    \"\"\"\ngood: 7\n"_el;
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        REQUIRE(document->hasErrors());
        REQUIRE_EQUAL(document->statements().count().toSizeT(), 3U);
        REQUIRE(document->statements().getRef(el::ItemIndex{1}).hasErrors());
        auto found = false;
        for (const auto &token : document->tokens()) {
            if (token->kind() == el::conf::syntax::TokenKind::MultiLineText && token->text() == "valid text"_el) {
                found = true;
            }
        }
        REQUIRE(found);
        const auto output = el::conf::syntax::Formatter{}.format(document).text();
        REQUIRE(output.contains("text: \"\"\"\n    bad\\q\n    valid text\n    \"\"\"\n"_el));
        REQUIRE(output.contains("Good"_el));
    }
    void testMissingCloseAndUnresolvedSections() {
        const el::String source = "[main]\ntext: \"\"\"\n    text\n[broken\nx: 1\n[other]\ny: 2\n"_el;
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        REQUIRE(document->hasErrors());
        REQUIRE_EQUAL(document->statements().count().toSizeT(), 6U);
        REQUIRE_FALSE(document->statements().getRef(el::ItemIndex{3}).namePath());
        REQUIRE_EQUAL(document->statements().last().namePath()->toText(), "other.y"_el);
        auto joined = el::StringEditor{};
        for (const auto &token : document->tokens()) {
            joined.append(token->text());
        }
        REQUIRE_EQUAL(el::String{joined}, source);
    }
    void testEmptyAndMissingValueAtEnd() {
        REQUIRE(el::conf::syntax::Parser{}.parseText({})->tokens().isEmpty());
        const auto document = el::conf::syntax::Parser{}.parseText("x:"_el);
        REQUIRE(document->hasErrors());
        REQUIRE_EQUAL(el::conf::syntax::Formatter{}.format(document).text(), "x:"_el);
    }
    void testStructuralErrorsAndLineLimits() {
        const auto source = "[valid]\nx: 1\n[broken.]\ny: 2\n[]\n[good]\nz: 3\n"_el;
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        REQUIRE(document->hasErrors());
        REQUIRE(document->statements().getRef(el::ItemIndex{2}).hasErrors());
        REQUIRE(document->statements().getRef(el::ItemIndex{4}).hasErrors());
        REQUIRE_FALSE(document->statements().getRef(el::ItemIndex{3}).namePath());
        REQUIRE_EQUAL(document->statements().last().namePath()->toText(), "good.z"_el);
        auto longLine = el::StringEditor{"[main]\nlong: \""_el};
        longLine.append(el::Char{U'a'}, el::CpCount{5000}).append("\"\ngood: 2\n"_el);
        const auto longDocument = el::conf::syntax::Parser{}.parseText(longLine);
        REQUIRE(longDocument->hasErrors());
        REQUIRE(longDocument->statements().getRef(el::ItemIndex{1}).hasErrors());
        REQUIRE_FALSE(longDocument->statements().getRef(el::ItemIndex{2}).hasErrors());
    }
    void testLocationsAfterRecovery() {
        const el::String source = "[a]\nbad: ???\ngood: 2\n"_el;
        const auto document = el::conf::syntax::Parser{}.parseText(source);
        auto position = el::CpIndex{};
        for (const auto &token : document->tokens()) {
            REQUIRE_EQUAL(token->location().position(), position);
            position += token->text().characterLength();
        }
        REQUIRE_FALSE(document->diagnostics().first().range.isEmpty());
    }
};
