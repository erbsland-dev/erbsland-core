// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(Text MultiLine)
class LexerStandardMultiLineTextTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    struct Line {
        el::text::String actualContent;   // The actual line content
        el::text::String actualTrailing;  // The trailing whitespace on the line
        el::text::String expectedContent; // The expected line content
    };
    using Lines = std::vector<Line>;

    enum class MultiLineStyle : uint8_t { Plain, WithCommentAfterOpenBracket, NoIndentOnEmptyLines };
    static constexpr auto cMultiLineStyles = std::array<MultiLineStyle, 3>{
        MultiLineStyle::Plain, MultiLineStyle::WithCommentAfterOpenBracket, MultiLineStyle::NoIndentOnEmptyLines};

    [[nodiscard]] static auto indentForPrefix(PrefixFormat prefixFormat) -> el::text::String {
        switch (prefixFormat) {
        case PrefixFormat::SameLine:
        case PrefixFormat::NextLinePattern1:
        case PrefixFormat::NextLinePattern1withComment:
            return cValueOnNextLineIndentationPattern1;
        case PrefixFormat::NextLinePattern2:
            return cValueOnNextLineIndentationPattern2;
        case PrefixFormat::NextLinePattern3:
            return cValueOnNextLineIndentationPattern3;
        default:
            throw std::logic_error("Prefix format not implemented.");
        }
    }

    [[nodiscard]] static auto createValueText(
        const Lines &testLines,
        const el::text::String &bracket,
        PrefixFormat prefixFormat,
        MultiLineStyle multiLineStyle) -> el::text::String {

        auto result = el::text::StringEditor{bracket};
        const auto indent = indentForPrefix(prefixFormat);
        switch (multiLineStyle) {
        case MultiLineStyle::Plain:
        case MultiLineStyle::NoIndentOnEmptyLines:
            result.append("\n"_el);
            for (const auto &line : testLines) {
                if (multiLineStyle != MultiLineStyle::NoIndentOnEmptyLines || !line.actualContent.isEmpty() ||
                    !line.actualTrailing.isEmpty()) {
                    result.append(indent);
                    result.append(line.actualContent);
                    result.append(line.actualTrailing);
                }
                result.append("\n"_el);
            }
            result.append(indent);
            result.append(bracket);
            break;
        case MultiLineStyle::WithCommentAfterOpenBracket:
            result.append(" "_el);
            result.append(cSimpleComment);
            result.append("\n"_el);
            for (const auto &line : testLines) {
                result.append(indent);
                result.append(line.actualContent);
                result.append(line.actualTrailing);
                result.append("\n"_el);
            }
            result.append(indent);
            result.append(bracket);
            break;
        default:
            throw std::logic_error("MultiLine style not implemented");
        }
        return result;
    }

    void verifyMultiLinePrefix(PrefixFormat prefixFormat, MultiLineStyle multiLineStyle) {
        WITH_CONTEXT(requireNextToken(TokenType::MultiLineTextOpen, "\"\"\""_el));
        switch (multiLineStyle) {
        case MultiLineStyle::Plain:
        case MultiLineStyle::NoIndentOnEmptyLines:
            WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
            break;
        case MultiLineStyle::WithCommentAfterOpenBracket:
            WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
            WITH_CONTEXT(requireNextToken(TokenType::Comment, cSimpleComment));
            WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
            break;
        default:
            throw std::logic_error("MultiLine style not implemented");
        }
    }

    void verifyMultiLineLines(const Lines &testLines, PrefixFormat prefixFormat, MultiLineStyle multiLineStyle) {
        const auto indent = indentForPrefix(prefixFormat);
        for (std::size_t i = 0; i < testLines.size(); ++i) {
            const auto &[actualContent, actualTrailing, expectedContent] = testLines[i];
            if (multiLineStyle != MultiLineStyle::NoIndentOnEmptyLines || !actualContent.isEmpty() ||
                !actualTrailing.isEmpty()) {
                WITH_CONTEXT(requireNextToken(TokenType::Indentation, indent));
            }
            if (!actualContent.isEmpty()) {
                WITH_CONTEXT(requireNextStringToken(TokenType::MultiLineText, expectedContent, actualContent));
            }
            if (!actualTrailing.isEmpty()) {
                WITH_CONTEXT(requireNextToken(TokenType::Spacing, actualTrailing));
            }
            WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        }
    }

    void verifyMultiLineSuffix(PrefixFormat prefixFormat) {
        const auto indent = indentForPrefix(prefixFormat);
        WITH_CONTEXT(requireNextToken(TokenType::Indentation, indent));
        WITH_CONTEXT(requireNextToken(TokenType::MultiLineTextClose, "\"\"\""_el));
    }

    void verifyMultiLineText(const Lines &testLines, PrefixFormat prefixFormat, MultiLineStyle multiLineStyle) {
        WITH_CONTEXT(verifyMultiLinePrefix(prefixFormat, multiLineStyle))
        WITH_CONTEXT(verifyMultiLineLines(testLines, prefixFormat, multiLineStyle));
        WITH_CONTEXT(verifyMultiLineSuffix(prefixFormat));
    }

    /// Verify valid multi-line tests.
    ///
    /// Expects a vector of lines and automatically iterates over many combinations of indentation styles.
    /// If the first line starts with a space or tab, only next line formats are tried.
    ///
    /// @param testLines The lines to test.
    ///
    void verifyValidMultiLineText(const Lines &testLines) {
        const auto bracket = el::text::String{"\"\"\""_el};
        for (auto prefixFormat : cPrefixFormats) {
            // Skip the same-line test if the first line starts with spacing.
            if (!testLines.empty() && !testLines[0].actualContent.isEmpty() &&
                (testLines[0].actualContent.charAt(el::text::StringSide::Front) == u8' ' ||
                    testLines[0].actualContent.charAt(el::text::StringSide::Front) == u8'\t') &&
                prefixFormat == PrefixFormat::SameLine) {
                continue;
            }
            for (auto suffixFormat : cSuffixPatterns) {
                for (auto multiLineStyle : cMultiLineStyles) {
                    auto valueText = createValueText(testLines, bracket, prefixFormat, multiLineStyle);
                    setupTokenGeneratorForValueTest(valueText, prefixFormat, suffixFormat);
                    WITH_CONTEXT(verifyPrefix(prefixFormat));
                    WITH_CONTEXT(verifyMultiLineText(testLines, prefixFormat, multiLineStyle));
                    WITH_CONTEXT(verifySuffix(suffixFormat));
                }
            }
        }
    }

    void testEmpty() { WITH_CONTEXT(verifyValidMultiLineText({})); }

    void testSingleLine() {
        const auto testLines = Lines{
            {.actualContent = "This is a single line of text."_el,
                .actualTrailing = ""_el,
                .expectedContent = "This is a single line of text."_el}};
        WITH_CONTEXT(verifyValidMultiLineText(testLines));
    }

    void testEmptyLineMiddle() {
        const auto testLines = Lines{
            {.actualContent = "The first line of text"_el,
                .actualTrailing = "     "_el,
                .expectedContent = "The first line of text"_el},
            {.actualContent = ""_el, .actualTrailing = ""_el, .expectedContent = ""_el},
            {.actualContent = "The last line of text"_el,
                .actualTrailing = "\t"_el,
                .expectedContent = "The last line of text"_el}};
        WITH_CONTEXT(verifyValidMultiLineText(testLines));
    }

    void testEmptyLineFirst() {
        const auto testLines = Lines{
            {.actualContent = ""_el, .actualTrailing = ""_el, .expectedContent = ""_el},
            {.actualContent = "The second line of text"_el,
                .actualTrailing = "   \t "_el,
                .expectedContent = "The second line of text"_el},
            {.actualContent = "The last line of text"_el,
                .actualTrailing = "\t  "_el,
                .expectedContent = "The last line of text"_el}};
        WITH_CONTEXT(verifyValidMultiLineText(testLines));
    }

    void testEmptyLineLast() {
        const auto testLines = Lines{
            {.actualContent = "The first line of text"_el,
                .actualTrailing = ""_el,
                .expectedContent = "The first line of text"_el},
            {.actualContent = "The second line of text"_el,
                .actualTrailing = ""_el,
                .expectedContent = "The second line of text"_el},
            {.actualContent = ""_el, .actualTrailing = ""_el, .expectedContent = ""_el},
        };
        WITH_CONTEXT(verifyValidMultiLineText(testLines));
    }

    void testEscapeSequences() {
        const auto testLines = Lines{
            {.actualContent = R"(\"\n\r\$\u{41}●🄴\u0041\\)"_el,
                .actualTrailing = ""_el,
                .expectedContent = "\"\n\r$\u0041●🄴\u0041\\"_el},
            {.actualContent = R"("" this is not the end)"_el,
                .actualTrailing = ""_el,
                .expectedContent = "\"\" this is not the end"_el},
            {.actualContent = R"(\""")"_el, .actualTrailing = ""_el, .expectedContent = "\"\"\""_el},
        };
        WITH_CONTEXT(verifyValidMultiLineText(testLines));
    }

    void testSpacing() {
        const auto testLines = Lines{
            {.actualContent = "    text"_el, .actualTrailing = "    "_el, .expectedContent = "    text"_el},
            {.actualContent = "        text"_el, .actualTrailing = "        "_el, .expectedContent = "        text"_el},
            {.actualContent = R"(text        \u{20})"_el,
                .actualTrailing = ""_el,
                .expectedContent = "text         "_el},
            {.actualContent = "  text"_el, .actualTrailing = "  "_el, .expectedContent = "  text"_el},
        };
        WITH_CONTEXT(verifyValidMultiLineText(testLines));
    }

    void testIgnoredIndentedEndSequence() {
        const auto testLines = Lines{
            {.actualContent = "text"_el, .actualTrailing = ""_el, .expectedContent = "text"_el},
            {.actualContent = " \"\"\""_el, .actualTrailing = ""_el, .expectedContent = " \"\"\""_el},
            {.actualContent = "text"_el, .actualTrailing = ""_el, .expectedContent = "text"_el},
            {.actualContent = "\t\"\"\""_el, .actualTrailing = ""_el, .expectedContent = "\t\"\"\""_el},
        };
        WITH_CONTEXT(verifyValidMultiLineText(testLines));
    }

    void testIndentationError() {
        // Simulate an error when the indentation of the second line differs from the previous one.
        setupTokenGenerator(
            join({cSectionLine, cValueStart, cValueOnSameLineSpacing, "\"\"\"\n    text\n  text\n    \"\"\"\n"_el}));
        WITH_CONTEXT(verifyPrefix(PrefixFormat::SameLine));
        WITH_CONTEXT(requireNextToken(TokenType::MultiLineTextOpen, "\"\"\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Indentation, "    "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::MultiLineText, "text"_el, "text"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::Indentation));
    }

    void testInvalidEscapeSequence() {
        // Simulate an error when an invalid escape sequence is encountered.
        setupTokenGenerator(
            join({cSectionLine, cValueStart, cValueOnSameLineSpacing, "\"\"\"\n    text\\\n    \"\"\"\n"_el}));
        WITH_CONTEXT(verifyPrefix(PrefixFormat::SameLine));
        WITH_CONTEXT(requireNextToken(TokenType::MultiLineTextOpen, "\"\"\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Indentation, "    "_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::Syntax));
    }

    void testEndInText() {
        // Simulate an error when the document ends in the middle of the text.
        setupTokenGenerator(join({cSectionLine, cValueStart, cValueOnSameLineSpacing, "\"\"\"\n    text"_el}));
        WITH_CONTEXT(verifyPrefix(PrefixFormat::SameLine));
        WITH_CONTEXT(requireNextToken(TokenType::MultiLineTextOpen, "\"\"\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Indentation, "    "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::MultiLineText, "text"_el, "text"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::UnexpectedEnd));
    }
};
