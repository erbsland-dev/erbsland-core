// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"

#include <erbsland/re/impl/diagnostics/AssemblerTokenizer.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/StdFormatForRegEx.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace el::re;
using namespace el::text::literals;
using namespace re_test::string_helper;
using impl::AssemblerToken;
using impl::AssemblerTokenizer;
using impl::Operation;
using impl::OperationModifier;

TESTED_TARGETS(AssemblerTokenizer)
TAGS(Diagnostics)
class AssemblerTokenizerTest final : public el::UnitTest {
public:
    std::vector<AssemblerToken> tokens;

    void setUp() override { tokens.clear(); }

    auto additionalErrorMessages() -> std::string override {
        try {
            if (tokens.empty()) {
                return "tokens: <empty>";
            }
            auto msg = std::format("tokens: {}\n", tokens.size());
            for (std::size_t i = 0; i < tokens.size(); ++i) {
                msg += std::format("  [{}] {}\n", i, tokens[i]);
            }
            return msg;
        } catch (const std::exception &exc) {
            return std::format("Unexpected exception while providing additional error messages: {}", exc.what());
        }
    }

    void requireTokenizeSuccess(const std::string_view line) {
        tokens.clear();
        AssemblerTokenizer tokenizer{StringEditor{line}};
        REQUIRE_NOTHROW(tokens = tokenizer.tokens());
    }

    void requireTokenizeFail(
        const std::string_view line,
        const ErrorCategory expectedCategory,
        const std::string_view expectedMessage,
        const std::optional<std::size_t> expectedColumn = {}) {

        tokens.clear();
        try {
            AssemblerTokenizer tokenizer{StringEditor{line}};
            (void)tokenizer.tokens();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), expectedCategory);
            REQUIRE_EQUAL(e.title(), "Failed to assemble regular expression"_el);
            REQUIRE_EQUAL(e.description(), StringEditor{expectedMessage});
            if (expectedColumn.has_value()) {
                REQUIRE_EQUAL(e.column().toSizeT() + 1U, expectedColumn.value());
            }
        }
        REQUIRE(tokens.empty());
    }

    void requireTokensCallTwiceFails(
        const std::string_view line, const ErrorCategory expectedCategory, const std::string_view expectedMessage) {

        AssemblerTokenizer tokenizer{StringEditor{line}};
        REQUIRE_EQUAL(tokenizer.tokens().size(), 1U);
        REQUIRE_THROWS_AS(RegExError, tokenizer.tokens());
        try {
            (void)tokenizer.tokens();
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), expectedCategory);
            REQUIRE_EQUAL(e.title(), "Internal regular-expression failure"_el);
            REQUIRE_EQUAL(e.description(), StringEditor{expectedMessage});
        }
    }

    void testTokensEmptyAndWhitespaceOnly() {
        WITH_CONTEXT(requireTokenizeSuccess(""));
        REQUIRE(tokens.empty());

        WITH_CONTEXT(requireTokenizeSuccess("    \t"));
        REQUIRE(tokens.empty());
    }

    void testTokensIntegerCommaMinusAndComment() {
        WITH_CONTEXT(requireTokenizeSuccess("1,2"));
        REQUIRE_EQUAL(tokens.size(), 3U);
        REQUIRE(tokens[0].isInteger());
        REQUIRE_EQUAL(tokens[0].getInteger(), 1U);
        REQUIRE_EQUAL(tokens[0].column(), el::unit::ColumnIndex{0U});

        REQUIRE(tokens[1].isComma());
        REQUIRE_EQUAL(tokens[1].column(), el::unit::ColumnIndex{1U});

        REQUIRE(tokens[2].isInteger());
        REQUIRE_EQUAL(tokens[2].getInteger(), 2U);
        REQUIRE_EQUAL(tokens[2].column(), el::unit::ColumnIndex{2U});

        WITH_CONTEXT(requireTokenizeSuccess("- 1"));
        REQUIRE_EQUAL(tokens.size(), 2U);
        REQUIRE(tokens[0].isMinus());
        REQUIRE_EQUAL(tokens[0].column(), el::unit::ColumnIndex{0U});
        REQUIRE(tokens[1].isInteger());
        REQUIRE_EQUAL(tokens[1].getInteger(), 1U);
        REQUIRE_EQUAL(tokens[1].column(), el::unit::ColumnIndex{2U});

        WITH_CONTEXT(requireTokenizeSuccess("1 ; this is a comment"));
        REQUIRE_EQUAL(tokens.size(), 1U);
        REQUIRE(tokens[0].isInteger());
        REQUIRE_EQUAL(tokens[0].getInteger(), 1U);
    }

    void testTokensTextLiteralAndEscapes() {
        WITH_CONTEXT(requireTokenizeSuccess("\"a\\n\\t\\\\\\\"b\""));
        REQUIRE_EQUAL(tokens.size(), 1U);
        REQUIRE(tokens[0].isText());
        REQUIRE_EQUAL(tokens[0].getText(), "a\n\t\\\"b"_el);
        REQUIRE_EQUAL(tokens[0].column(), el::unit::ColumnIndex{0U});
    }

    void testTokensCharacterLiteralAndEscapes() {
        WITH_CONTEXT(requireTokenizeSuccess("'a'"));
        REQUIRE_EQUAL(tokens.size(), 1U);
        REQUIRE(tokens[0].isChar());
        REQUIRE_EQUAL(tokens[0].getInteger(), static_cast<uint32_t>(U'a'));

        WITH_CONTEXT(requireTokenizeSuccess("'😄'"));
        REQUIRE_EQUAL(tokens.size(), 1U);
        REQUIRE(tokens[0].isChar());
        REQUIRE_EQUAL(tokens[0].getInteger(), static_cast<uint32_t>(U'😄'));

        WITH_CONTEXT(requireTokenizeSuccess("'\\n'"));
        REQUIRE_EQUAL(tokens.size(), 1U);
        REQUIRE(tokens[0].isChar());
        REQUIRE_EQUAL(tokens[0].getInteger(), static_cast<uint32_t>(U'\n'));
    }

    void testTokensKeywordBooleanModifierOperationLabelIdentifierOffsetCommand() {
        WITH_CONTEXT(requireTokenizeSuccess("TRUE false ci not assert skip add start stop match"));
        REQUIRE_EQUAL(tokens.size(), 10U);
        REQUIRE(tokens[0].isBoolean());
        REQUIRE_EQUAL(tokens[0].getBoolean(), true);
        REQUIRE(tokens[1].isBoolean());
        REQUIRE_EQUAL(tokens[1].getBoolean(), false);

        REQUIRE(tokens[2].isModifier());
        REQUIRE_EQUAL(tokens[2].getModifier(), OperationModifier::CaseInsensitive);
        REQUIRE(tokens[3].isModifier());
        REQUIRE_EQUAL(tokens[3].getModifier(), OperationModifier::Negated);
        REQUIRE(tokens[4].isModifier());
        REQUIRE_EQUAL(tokens[4].getModifier(), OperationModifier::Assert);
        REQUIRE(tokens[5].isModifier());
        REQUIRE_EQUAL(tokens[5].getModifier(), OperationModifier::Skip);
        REQUIRE(tokens[6].isModifier());
        REQUIRE_EQUAL(tokens[6].getModifier(), OperationModifier::Add);
        REQUIRE(tokens[7].isModifier());
        REQUIRE_EQUAL(tokens[7].getModifier(), OperationModifier::Start);
        REQUIRE(tokens[8].isModifier());
        REQUIRE_EQUAL(tokens[8].getModifier(), OperationModifier::Stop);

        REQUIRE(tokens[9].isOperation());
        REQUIRE_EQUAL(tokens[9].getOperation(), Operation::Match);

        WITH_CONTEXT(requireTokenizeSuccess("abc: MATCH"));
        REQUIRE_EQUAL(tokens.size(), 2U);
        REQUIRE(tokens[0].isLabel());
        REQUIRE_EQUAL(tokens[0].getText(), "abc"_el);
        REQUIRE(tokens[1].isOperation());

        WITH_CONTEXT(requireTokenizeSuccess("%Label_1 $1a2b &Id_1 .data"));
        REQUIRE_EQUAL(tokens.size(), 4U);
        REQUIRE(tokens[0].isLabel());
        REQUIRE_EQUAL(tokens[0].getText(), "label_1"_el);
        REQUIRE(tokens[1].isOffset());
        REQUIRE_EQUAL(tokens[1].getInteger(), 0x1a2bU);
        REQUIRE(tokens[2].isIdentifier());
        REQUIRE_EQUAL(tokens[2].getText(), "Id_1"_el);
        REQUIRE(tokens[3].isCommand());
        REQUIRE_EQUAL(tokens[3].getText(), "data"_el);
    }

    void testErrorRepeatedTokensCallThrowsInternalError() {
        WITH_CONTEXT(requireTokensCallTwiceFails("1", ErrorCategory::Internal, "Don't call 'tokens()' twice"));
    }

    void testErrorUnexpectedCharacter() {
        WITH_CONTEXT(requireTokenizeFail("1x", ErrorCategory::Assembler, "Unexpected character", 2U));
    }

    void testErrorUnknownKeyword() {
        WITH_CONTEXT(requireTokenizeFail("foo", ErrorCategory::Assembler, "Unknown keyword 'foo'", 3U));
    }

    void testErrorIntegerTooLarge() {
        WITH_CONTEXT(requireTokenizeFail("1234567890", ErrorCategory::Assembler, "Integer too large", 10U));
    }

    void testErrorTextLiteralCases() {
        WITH_CONTEXT(requireTokenizeFail("\"abc", ErrorCategory::Assembler, "Unterminated text literal"));
        WITH_CONTEXT(requireTokenizeFail("\"\\x\"", ErrorCategory::Assembler, "Invalid escape sequence"));

        std::string longText;
        longText.reserve(2200);
        longText.push_back('"');
        for (std::size_t i = 0; i < 700; ++i) {
            longText.append(bytesToStdString({0xE2, 0x82, 0xAC})); // U+20AC, 3 bytes per code point.
        }
        longText.push_back('"');
        WITH_CONTEXT(requireTokenizeFail(longText, ErrorCategory::Assembler, "Text literal too long"));

        const auto invalidCharText = std::string{"\""} + std::string{static_cast<char>(0x01)} + std::string{"\""};
        WITH_CONTEXT(
            requireTokenizeFail(invalidCharText, ErrorCategory::Assembler, "Invalid character in text literal"));
    }

    void testErrorCharacterLiteralCases() {
        WITH_CONTEXT(
            requireTokenizeFail("'ab'", ErrorCategory::Assembler, "Character literal must have a single code-point"));
        WITH_CONTEXT(requireTokenizeFail("'", ErrorCategory::Assembler, "Unterminated character literal"));
        WITH_CONTEXT(requireTokenizeFail("'\\x'", ErrorCategory::Assembler, "Invalid escape sequence"));

        const auto invalidCharText = std::string{"'"} + std::string{static_cast<char>(0x01)} + std::string{"'"};
        WITH_CONTEXT(
            requireTokenizeFail(invalidCharText, ErrorCategory::Assembler, "Invalid character in character literal"));
    }

    void testErrorLabelOffsetIdentifierCommandCases() {
        WITH_CONTEXT(requireTokenizeFail("%", ErrorCategory::Assembler, "Expected a label after '%'", 1U));
        WITH_CONTEXT(
            requireTokenizeFail("%1", ErrorCategory::Assembler, "Unexpected character after '%'. Expected a letter"));
        WITH_CONTEXT(requireTokenizeFail("$", ErrorCategory::Assembler, "Missing offset after '$'"));
        WITH_CONTEXT(requireTokenizeFail("$123456789", ErrorCategory::Assembler, "Offset has too many digits"));
        WITH_CONTEXT(requireTokenizeFail("&", ErrorCategory::Assembler, "Missing identifier after '&'"));
        WITH_CONTEXT(requireTokenizeFail(".", ErrorCategory::Assembler, "Missing command after '.'"));
        WITH_CONTEXT(requireTokenizeFail(".a1", ErrorCategory::Assembler, "Unexpected character"));
    }

    void testErrorKeywordOrLabelTooLong() {
        const std::string keyword(AssemblerTokenizer::maxIdentifierLength + 1U, 'a');
        WITH_CONTEXT(requireTokenizeFail(keyword, ErrorCategory::Assembler, "Keyword or label too long"));
    }

    void testErrorLabelTooLong() {
        const std::string label = std::string{"%"} + std::string(AssemblerTokenizer::maxIdentifierLength + 1U, 'a');
        WITH_CONTEXT(requireTokenizeFail(label, ErrorCategory::Assembler, "label too long"));
    }

    void testErrorIdentifierTooLong() {
        const std::string identifier =
            std::string{"&"} + std::string(AssemblerTokenizer::maxIdentifierLength + 1U, 'a');
        WITH_CONTEXT(requireTokenizeFail(identifier, ErrorCategory::Assembler, "identifier too long"));
    }

    void testErrorCommandTooLong() {
        const std::string command = std::string{"."} + std::string(AssemblerTokenizer::maxIdentifierLength + 1U, 'a');
        WITH_CONTEXT(requireTokenizeFail(command, ErrorCategory::Assembler, "command too long"));
    }

    void testMalformedUtf8InCommentBecomesReplacement() {
        std::string line;
        line.push_back(';');
        line.push_back(static_cast<char>(0xFF));
        AssemblerTokenizer tokenizer{StringEditor{line}};
        REQUIRE_NOTHROW(tokens = tokenizer.tokens());
        REQUIRE(tokens.empty());
    }
};
