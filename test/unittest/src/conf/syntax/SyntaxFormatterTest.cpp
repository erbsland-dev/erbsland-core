// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/syntax/Formatter.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
TESTED_TARGETS(SyntaxFormatter)
class SyntaxFormatterTest final : public el::UnitTest {
private:
    static constexpr auto cTextPath = el::StringLiteral{"a.x"};
    auto format(const el::String &text, el::conf::syntax::Format options = {}) -> el::String {
        return el::conf::syntax::Formatter{std::move(options)}
            .format(el::conf::syntax::Parser{}.parseText(text))
            .text();
    }
    auto simple() -> el::conf::syntax::Format {
        auto result = el::conf::syntax::Format{};
        result.clearRules();
        auto style = result.baseStyle();
        style.setSectionIndent(el::CpCount{})
            .setSectionSuffix(el::CpCount{})
            .setSeparatorPositionMode(el::conf::syntax::PositionMode::Spaces)
            .setSeparatorPosition(el::CpCount{});
        result.setBaseStyle(style);
        return result;
    }

public:
    void testNullDocument() { REQUIRE_THROWS_AS(el::err::ParameterFailure, el::conf::syntax::Formatter{}.format({})); }

    void testFormattingDiagnosticsLeaveSnapshotUnchanged() {
        const auto document = el::conf::syntax::Parser{}.parseText("@signature: \"sample\"\n[a]\nx:1\n"_el);
        REQUIRE(document->diagnostics().isEmpty());
        const auto result = el::conf::syntax::Formatter{}.format(document);
        REQUIRE_EQUAL(result.diagnostics().count(), el::ItemCount{1});
        REQUIRE(result.diagnostics().first().category == el::conf::ConfErrorCategory::Signature);
        REQUIRE_EQUAL(
            result.diagnostics().first().message, "Formatting changed signed source; re-sign the document."_el);
        REQUIRE(document->diagnostics().isEmpty());
    }

    void testNamesSpacingAndSemanticPreservation() {
        const auto source = "[my_section]\r\nmy_value=0x0a # comment\r\ntext:\"untouched_text\""_el;
        const auto output = format(source, simple());
        REQUIRE_EQUAL(output, "[ My Section ]\r\nMy Value: 0x0a # comment\r\nText: \"untouched_text\""_el);
        REQUIRE_EQUAL(format(output, simple()), output);
        auto parsed = el::conf::Parser{}.parseTextOrThrow(output);
        REQUIRE_EQUAL(parsed->getInteger("my_section.my_value"_el), 10);
        REQUIRE_EQUAL(parsed->getText("my_section.text"_el), "untouched_text"_el);
    }
    void testValuePlacementPoliciesAndSemanticPreservation() {
        using namespace el::conf::syntax;
        REQUIRE_EQUAL(FormatStyle{}.valueWrapping(), ValueWrapping::NextLineIfFits);
        auto options = simple();
        auto style = options.baseStyle();
        style.setMaximumLineLength(el::CpCount{24}).setValueWrapping(ValueWrapping::NextLineIfFits);
        options.setBaseStyle(style);
        const auto input = "[a]\nlong_name: \"1234567890\" # note\n"_el;
        const auto output = format(input, options);
        REQUIRE(output.contains("Long Name:\n    \"1234567890\" # note"_el));
        REQUIRE_EQUAL(format(output, options), output);
        REQUIRE_EQUAL(el::conf::Parser{}.parseTextOrThrow(output)->getText("a.long_name"_el), "1234567890"_el);
        style.setValueWrapping(ValueWrapping::SameLine);
        options.setBaseStyle(style);
        REQUIRE(format(output, options).contains("Long Name: \"1234567890\" # note"_el));
        style.setValueWrapping(ValueWrapping::NextLine);
        options.setBaseStyle(style);
        REQUIRE_EQUAL(format("[a]\nx: 1"_el, options), "[ A ]\nX:\n    1"_el);
        style.setValueWrapping(ValueWrapping::Keep);
        options.setBaseStyle(style);
        REQUIRE_EQUAL(format("[a]\nx:\n  1\n"_el, options), "[ A ]\nX:\n    1\n"_el);
        REQUIRE_EQUAL(format("[a]\nx: 1\n"_el, options), "[ A ]\nX: 1\n"_el);
        style.setMaximumLineLength(el::CpCount{10}).setValueWrapping(ValueWrapping::NextLineIfFits);
        options.setBaseStyle(style);
        const auto tooLong = "[a]\nx: \"a value longer than the line\"\n"_el;
        REQUIRE(format(tooLong, options).contains("X: \"a value longer than the line\""_el));
        style.setValueWrapping(ValueWrapping::WhenTooLong);
        options.setBaseStyle(style);
        REQUIRE(format(tooLong, options).contains("X:\n    \"a value longer than the line\""_el));
    }

    void testWrappingBoundaryUnicodeAndCompactVectors() {
        using namespace el::conf::syntax;
        auto options = simple();
        auto style = options.baseStyle();
        style.setMaximumLineLength(el::CpCount{15});
        options.setBaseStyle(style);
        REQUIRE_EQUAL(format("[a]\r\nlong_name: \"界界\""_el, options), "[ A ]\r\nLong Name: \"界界\""_el);
        style.setMaximumLineLength(el::CpCount{14});
        options.setBaseStyle(style);
        const auto wrapped = format("[a]\r\nlong_name: \"界界\""_el, options);
        REQUIRE_EQUAL(wrapped, "[ A ]\r\nLong Name:\r\n    \"界界\""_el);
        REQUIRE_EQUAL(format(wrapped, options), wrapped);
        style.setVectorStyle(VectorStyle::Compact).setMaximumLineLength(el::CpCount{16});
        options.setBaseStyle(style);
        REQUIRE_EQUAL(format("[a]\nlong_name: 1, 2, 3\n"_el, options), "[ A ]\nLong Name:\n    1, 2, 3\n"_el);
    }

    void testValueWrappingPreservesCommentsAndLiteralPayloads() {
        using namespace el::conf::syntax;
        auto options = simple();
        auto style = options.baseStyle();
        style.setValueWrapping(ValueWrapping::NextLine);
        options.setBaseStyle(style);
        const auto commented = format("[a]\nx: # attached\n    12\n"_el, options);
        REQUIRE(commented.contains("X: # attached\n    12"_el));
        REQUIRE_EQUAL(format(commented, options), commented);
        const auto source = "[a]\nx: \"\"\"\n    original payload\n    \"\"\"\n"_el;
        const auto output = format(source, options);
        REQUIRE(output.contains("X: \"\"\""_el));
        REQUIRE_EQUAL(
            el::conf::Parser{}.parseTextOrThrow(source)->getText(cTextPath),
            el::conf::Parser{}.parseTextOrThrow(output)->getText(cTextPath));
    }

    void testCenteredListSection() {
        auto options = simple();
        auto style = options.baseStyle();
        style.setMaximumLineLength(el::CpCount{20})
            .setSectionIndent(el::CpCount::infinite())
            .setSectionSuffix(el::CpCount::infinite());
        options.setBaseStyle(style);
        REQUIRE_EQUAL(format("*[list]\n"_el, options), "-----*[ List ]*-----\n"_el);
    }
    void testVectorReflowAndMatrixAlignment() {
        using namespace el::conf::syntax;
        auto options = simple();
        auto style = options.baseStyle();
        style.setVectorStyle(VectorStyle::Multiline);
        options.setBaseStyle(style);
        REQUIRE_EQUAL(format("[a]\nx:1,2,3\n"_el, options), "[ A ]\nX:\n    * 1\n    * 2\n    * 3\n"_el);
        style.setVectorStyle(VectorStyle::Compact);
        options.setBaseStyle(style);
        REQUIRE_EQUAL(format("[a]\nx:\n    * 1\n    * 2\n"_el, options), "[ A ]\nX: 1, 2\n"_el);
        REQUIRE_EQUAL(
            format("[a]\nx:\n    * 1,  22\n    * 333,4\n"_el, options), "[ A ]\nX:\n    * 1,  22\n    * 333,4\n"_el);
        style.setMatrixStyle(MatrixStyle::Align);
        options.setBaseStyle(style);
        REQUIRE_EQUAL(
            format("[a]\nx:\n    * 1,22\n    * 333,4\n"_el, options), "[ A ]\nX:\n    * 1,   22\n    * 333, 4\n"_el);
    }
    void testMultilinePayloadAndOptInText() {
        using namespace el::conf::syntax;
        const auto source = "[a]\nx: \"\"\"\n  content\n    relative\n  \"\"\"\nkind:\"section_list\"\n"_el;
        auto options = simple();
        options.addRule({Selector{}.addPath("a.kind"_el), FormatStylePatch{}.setTextIdentifierCase(WordCase::Title)});
        const auto output = format(source, options);
        REQUIRE(output.contains("    content\n      relative\n    \"\"\"\n"_el));
        REQUIRE(output.contains("Kind: \"Section List\""_el));
        const auto before = el::conf::Parser{}.parseTextOrThrow(source);
        const auto after = el::conf::Parser{}.parseTextOrThrow(output);
        REQUIRE_EQUAL(before->getText(cTextPath), after->getText(cTextPath));
        REQUIRE_EQUAL(format(output, options), output);
    }
    void testSingletonListAndIndependentNameSeparator() {
        using namespace el::conf::syntax;
        auto options = simple();
        auto style = options.baseStyle();
        style.setVectorStyle(VectorStyle::Compact)
            .setValueNameCase(WordCase::Keep)
            .setValueNameSeparator(WordSeparator::Underscore);
        options.setBaseStyle(style);
        const auto output = format("[a]\nMixed Name:\n    * 1\n"_el, options);
        REQUIRE(output.contains("Mixed_Name:\n    * 1\n"_el));
        REQUIRE_EQUAL(el::conf::Parser{}.parseTextOrThrow(output)->getInteger("a.mixed_name"_el), 1);
        REQUIRE_EQUAL(format(output, options), output);
    }
};
