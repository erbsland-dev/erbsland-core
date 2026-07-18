// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RegExBase.hpp"

#include <erbsland/text/u8/U8StringConstIterator.hpp>

class RegExCharClassBase : public RegExBase {
public:
    String patternFormat;
    String pattern;
    String textFormat;
    StringEditor textChar;

    auto additionalErrorMessages() -> std::string override {
        auto result = RegExBase::additionalErrorMessages();
        constexpr auto maximumWidth = el::unit::CpLength{200U};
        result += std::format("patternFormat: \"{}\"\n", patternFormat.toSafeString(maximumWidth));
        result += std::format("pattern: \"{}\"\n", pattern.toSafeString(maximumWidth));
        result += std::format("textFormat: \"{}\"\n", textFormat.toSafeString(maximumWidth));
        result += std::format("textChar: \"{}\"\n", textChar.toSafeString(maximumWidth));
        return result;
    }

    /// A test case for character class matching.
    struct CharClassTestCase {
        String pattern;
        std::vector<char32_t> matches;
        std::vector<char32_t> notMatches;
    };
    /// A list of test cases for character class matching.
    using CharClassTestCases = std::vector<CharClassTestCase>;

    /// A list of expected match locations.
    using ExpectedMatchLocations = std::vector<CaptureRange>;

    /// The prepared pattern for testing.
    struct PreparedPattern {
        StringEditor pattern;
        StringEditor text;
        ExpectedMatchLocations expectedMatchLocations;
        std::vector<std::string> expectedGroupLines;
    };

    /// Build a pattern from a given format string.
    /// @param patternFormatStr The format, using Unicode `■` as placeholder, where to insert `pattern`.
    /// @param patternStr The pattern to insert into the format string.
    /// @param textFormatStr The format, using Unicode `●` as placeholder, where to insert `textChar`,
    ///     using Unicode `⟪` to mark the begin of a match and `⟫` to mark the end of a match.
    /// @param testedChar The character to match in the pattern.
    [[nodiscard]] auto buildPattern(
        const String &patternFormatStr,
        const String &patternStr,
        const String &textFormatStr,
        const el::text::Char testedChar) -> PreparedPattern {

        try {
            PreparedPattern result;
            for (const auto c : patternFormatStr) {
                if (c == U'■') {
                    result.pattern.append(patternStr);
                } else {
                    result.pattern.append(c);
                }
            }
            std::size_t begin;
            StringEditor capturedText;
            enum class State { Outside, Inside } state = State::Outside;
            bool insertedChar = false;
            for (const auto c : textFormatStr) {
                if (c == U'⟪') {
                    REQUIRE(state == State::Outside);
                    begin = result.text.length().toSizeT();
                    state = State::Inside;
                } else if (c == U'⟫') {
                    REQUIRE(state == State::Inside);
                    const auto end = result.text.length().toSizeT();
                    result.expectedMatchLocations.emplace_back(begin, end);
                    result.expectedGroupLines.emplace_back(
                        std::format(
                            "00: {:04}-{:04} '{}'",
                            begin,
                            end,
                            capturedText.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii)));
                    capturedText.clear();
                    state = State::Outside;
                } else if (c == U'●') {
                    REQUIRE(state == State::Inside);
                    result.text.append(testedChar);
                    capturedText.append(testedChar);
                    insertedChar = true;
                } else {
                    result.text.append(c);
                    if (state == State::Inside) {
                        capturedText.append(c);
                    }
                }
            }
            REQUIRE(state == State::Outside);                // sanity
            REQUIRE(insertedChar);                           // sanity
            REQUIRE(!result.expectedMatchLocations.empty()); // sanity
            return result;
        } catch (const el::AssertFailed &) {
            auto testedCharText = StringEditor{"'"_el};
            testedCharText.append(testedChar);
            testedCharText.append('\'');
            consoleWriteLine(
                std::format(
                    "Sanity checks failed for buildPattern(patternFormatStr=\"{}\", patternStr=\"{}\","
                    "textFormatStr=\"{}\", testedChar={})",
                    patternFormat.toSafeString(el::unit::CpLength{200U}),
                    patternStr.toSafeString(el::unit::CpLength{200U}),
                    textFormat.toSafeString(el::unit::CpLength{200U}),
                    testedCharText.toSafeString(el::unit::CpLength{200U})));
            throw;
        }
    }

    struct PatternTestCase {
        String patternFormat;
        std::vector<String> textFormats;
    };
    using PatternTestCases = std::vector<PatternTestCase>;

    void requireCharClassMatch(
        const CharClassTestCases &charClassTestCases,
        const PatternTestCases &patternTestCases,
        const std::function<void(const PreparedPattern &)> &testMatchFn) {

        for (const auto &testPattern : patternTestCases) {
            patternFormat = testPattern.patternFormat;
            for (const auto &text : testPattern.textFormats) {
                this->textFormat = text;
                for (const auto &testCase : charClassTestCases) {
                    pattern = testCase.pattern;
                    for (const auto c : testCase.matches) {
                        textChar.clear();
                        textChar.append(el::text::Char{c});
                        auto prepared =
                            buildPattern(testPattern.patternFormat, testCase.pattern, text, el::text::Char{c});
                        this->text = prepared.text;
                        testMatchFn(prepared);
                    }
                }
            }
        }
    }

    void requireCharClassNotMatch(
        const CharClassTestCases &charClassTestCases,
        const PatternTestCases &patternTestCases,
        const std::function<void(const PreparedPattern &)> &testMatchFn) {

        for (const auto &testPattern : patternTestCases) {
            patternFormat = testPattern.patternFormat;
            for (const auto &text : testPattern.textFormats) {
                this->textFormat = text;
                for (const auto &testCase : charClassTestCases) {
                    pattern = testCase.pattern;
                    for (const auto c : testCase.notMatches) {
                        textChar.clear();
                        textChar.append(el::text::Char{c});
                        auto prepared =
                            buildPattern(testPattern.patternFormat, testCase.pattern, text, el::text::Char{c});
                        testMatchFn(prepared);
                    }
                }
            }
        }
    }
};
