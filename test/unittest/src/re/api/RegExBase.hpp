// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/diagnostics/Disassembler.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace el::re;

/// Provides common state and assertions for regular-expression API tests.
/// @notest{Provides shared test infrastructure.}
class RegExBase : public re_test::TestHelper {
public:
    String pattern;                      ///< The most recently compiled pattern.
    Flags flags;                         ///< The flags used to compile the pattern.
    Settings settings;                   ///< The settings used to compile the pattern.
    RegExPtr regex;                      ///< The compiled regular expression.
    String text;                         ///< The most recently matched text.
    MatchBasePtr lastMatch;              ///< The most recently produced match.
    std::vector<std::string> matchLines; ///< Diagnostic lines for the collected matches.
    String textWithReplacements;         ///< The most recent replacement result.

    void setUp() override {
        pattern = {};
        flags = {};
        settings = {};
        regex = nullptr;
        lastMatch = nullptr;
        text = {};
        matchLines = {};
        textWithReplacements = {};
    }

    /// Create diagnostic lines for the capture groups in the last match.
    [[nodiscard]] auto createGroupLines() -> std::vector<std::string> {
        if (lastMatch == nullptr) {
            return {};
        }
        const auto match = std::dynamic_pointer_cast<Match>(lastMatch);
        REQUIRE(match != nullptr);
        std::vector<std::string> result;
        for (auto index = std::size_t{}; index < lastMatch->groupCount(); ++index) {
            const auto groupIndex = static_cast<CaptureGroupIndex>(index);
            result.emplace_back(
                std::format(
                    "{:02}: {:04}-{:04} '{}'",
                    index,
                    lastMatch->begin(groupIndex),
                    lastMatch->end(groupIndex),
                    match->content(groupIndex)
                        .toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii)));
        }
        return result;
    }

    auto additionalErrorMessages() -> std::string override {
        try {
            std::string message;
            message += std::format("pattern: \"{}\"\n", pattern.toSafeString(el::unit::CpLength{200U}));
            message += std::format("flags: {}\n", flags);
            message += std::format("settings: {}\n", settings);
            if (regex == nullptr) {
                message += "regex: <null>\n";
            } else {
                for (const auto &line : diagnostics::Disassembler{regex}.disassemble()) {
                    message += std::format("{}\n", line);
                }
            }
            message += std::format("text: \"{}\"\n", text.toSafeString(el::unit::CpLength{200U}));
            if (lastMatch == nullptr) {
                message += "lastMatch: <null>\n";
            } else {
                message += "captureGroups = \n";
                for (const auto &line : createGroupLines()) {
                    message += line + '\n';
                }
            }
            if (!matchLines.empty()) {
                message += "matchLines = \n";
                for (const auto &line : matchLines) {
                    message += line + '\n';
                }
            }
            message += std::format(
                "textWithReplacements: \"{}\"\n", textWithReplacements.toSafeString(el::unit::CpLength{200U}));
            return message;
        } catch (...) {
            return "Unexpected exception while formatting diagnostics.";
        }
    }

    /// Require that a pattern compiles successfully.
    void requireCompile(const String &sourcePattern, const Flags sourceFlags = {}, const Settings sourceSettings = {}) {
        pattern = sourcePattern;
        flags = sourceFlags;
        settings = sourceSettings;
        REQUIRE_NOTHROW(regex = RegEx::compile(sourcePattern, sourceFlags, sourceSettings));
        REQUIRE(regex != nullptr);
    }

    /// Require that compiling a pattern fails.
    void requireCompileFail(
        const String &sourcePattern, const Flags sourceFlags = {}, const Settings sourceSettings = {}) {
        pattern = sourcePattern;
        flags = sourceFlags;
        settings = sourceSettings;
        REQUIRE_THROWS(regex = RegEx::compile(sourcePattern, sourceFlags, sourceSettings));
    }

    /// Require that the pattern matches a subject.
    void requireMatch(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->match(subject));
        REQUIRE(lastMatch != nullptr);
    }

    /// Require that the pattern does not match a subject.
    void requireNoMatch(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->match(subject));
        REQUIRE(lastMatch == nullptr);
    }

    /// Require that the pattern does not match any subject.
    void requireNoMatch(const std::vector<String> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoMatch(testCase));
        }
    }

    /// Specifies one expected match without explicit capture groups.
    struct NoCaptureTestCase {
        String text;                                        ///< The subject text.
        InputPosition begin = 0;                            ///< The expected match start.
        std::optional<String> expectedMatch = std::nullopt; ///< An optional expected matched text.
    };
    /// Defines a list of no-capture test cases.
    using NoneCapGroupTestCases = std::vector<NoCaptureTestCase>;

    /// Require a match with only the implicit full-match capture group.
    void requireMatchWithNoneCapGroups(
        const String &subject,
        const InputPosition begin = 0,
        const std::optional<String> &expectedMatch = std::nullopt) {
        const auto expected = expectedMatch.value_or(subject);
        const auto expectedLine = std::format(
            "00: {:04}-{:04} '{}'",
            begin,
            begin + expected.length().toSizeT(),
            expected.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii));
        WITH_CONTEXT(requireMatch(subject));
        WITH_CONTEXT(requireGroups({expectedLine}));
    }

    /// Require the implicit capture group for every test case.
    void requireMatchWithNoneCapGroups(const std::vector<NoCaptureTestCase> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireMatchWithNoneCapGroups(testCase.text, testCase.begin, testCase.expectedMatch));
        }
    }

    /// Require that the pattern fully matches a subject.
    void requireFullMatch(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->fullMatch(subject));
        REQUIRE(lastMatch != nullptr);
    }

    /// Require that the pattern does not fully match a subject.
    void requireNoFullMatch(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->fullMatch(subject));
        REQUIRE(lastMatch == nullptr);
    }

    /// Require that the pattern does not fully match any subject.
    void requireNoFullMatch(const std::vector<String> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoFullMatch(testCase));
        }
    }

    /// Require a full match with only the implicit capture group.
    void requireFullMatchWithNoCaptures(const String &subject) {
        const auto expectedLine = std::format(
            "00: {:04}-{:04} '{}'",
            0,
            subject.length().toSizeT(),
            subject.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii));
        WITH_CONTEXT(requireFullMatch(subject));
        WITH_CONTEXT(requireGroups({expectedLine}));
    }

    /// Require an implicit full-match capture group for every test case.
    void requireFullMatchWithNoCaptures(const std::vector<String> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireFullMatchWithNoCaptures(testCase));
        }
    }

    /// Require that the pattern finds a first match in a subject.
    void requireFindFirst(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->findFirst(subject));
    }

    /// Require that the pattern finds no first match in a subject.
    void requireNoFindFirst(const String &subject) {
        requireFindFirst(subject);
        REQUIRE(lastMatch == nullptr);
    }

    /// Require that the pattern finds no first match in any subject.
    void requireNoFindFirst(const std::vector<String> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoFindFirst(testCase));
        }
    }

    /// Require the first match with only the implicit capture group.
    void requireFindFirstNoCaptures(
        const String &subject,
        const InputPosition begin = 0,
        const std::optional<String> &expectedMatch = std::nullopt) {
        const auto expected = expectedMatch.value_or(subject);
        const auto expectedLine = std::format(
            "00: {:04}-{:04} '{}'",
            begin,
            begin + expected.length().toSizeT(),
            expected.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii));
        WITH_CONTEXT(requireFindFirst(subject));
        WITH_CONTEXT(requireGroups({expectedLine}));
    }

    /// Require implicit first-match capture groups for every test case.
    void requireFindFirstNoCaptures(const std::vector<NoCaptureTestCase> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireFindFirstNoCaptures(testCase.text, testCase.begin, testCase.expectedMatch));
        }
    }

    /// Require that the pattern finds one or more matches in a subject.
    void requireFindAll(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        auto matchCount = 0;
        matchLines.clear();
        for (const auto &match : regex->findAll(subject)) {
            ++matchCount;
            REQUIRE(match != nullptr);
            lastMatch = match;
            matchLines.emplace_back(std::format("Match {:02}:", matchCount));
            for (const auto &line : createGroupLines()) {
                matchLines.emplace_back(line);
            }
        }
        REQUIRE_GREATER(matchCount, 0);
    }

    /// Require that the pattern finds no matches in a subject.
    void requireNoFindAll(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        auto matchCount = 0;
        for ([[maybe_unused]] const auto &match : regex->findAll(subject)) {
            ++matchCount;
        }
        REQUIRE_EQUAL(matchCount, 0);
    }

    /// Require that the pattern finds no matches in any subject.
    void requireNoFindAll(const std::vector<String> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoFindAll(testCase));
        }
    }

    /// Require that collecting matches yields one or more matches.
    void requireCollectAll(const String &subject) {
        text = subject;
        REQUIRE(regex != nullptr);
        const auto matches = regex->collectAll(subject);
        REQUIRE_GREATER(matches.size(), 0);
        auto matchCount = 1;
        matchLines.clear();
        for (const auto &match : matches) {
            REQUIRE(match != nullptr);
            lastMatch = match;
            matchLines.emplace_back(std::format("Match {:02}:", matchCount++));
            for (const auto &line : createGroupLines()) {
                matchLines.emplace_back(line);
            }
        }
    }

    /// Specifies one expected replacement result.
    struct ReplaceTestCase {
        String text;                  ///< The subject text.
        String replacementExpression; ///< The replacement expression.
        String expectedResult;        ///< The expected replaced text.
    };
    /// Defines a list of replacement test cases.
    using ReplaceTestCases = std::vector<ReplaceTestCase>;

    /// Require that replacing all matches succeeds.
    void requireReplaceAll(const String &subject, const String &replacementExpression) {
        text = subject;
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(textWithReplacements = regex->replaceAll(subject, replacementExpression));
    }

    /// Require the expected replacement result for every test case.
    void requireReplaceAll(const ReplaceTestCases &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireReplaceAll(testCase.text, testCase.replacementExpression));
            REQUIRE_EQUAL(textWithReplacements, testCase.expectedResult);
        }
    }

    /// Require that the last match has the expected capture-group diagnostics.
    void requireGroups(const std::vector<std::string> &expectedGroups) {
        WITH_CONTEXT(requireLines(createGroupLines(), expectedGroups));
    }
};
