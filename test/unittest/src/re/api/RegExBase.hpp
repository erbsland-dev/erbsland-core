// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/diagnostics/Disassembler.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace el::re;

class RegExBase : public re_test::TestHelper {
public:
    String pattern;
    Flags flags;
    Settings settings;
    RegExPtr regex;
    String text;
    MatchBasePtr lastMatch;
    std::vector<std::string> matchLines;
    String textWithReplacements;

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

    void requireCompile(
        const StringView &sourcePattern, const Flags sourceFlags = {}, const Settings sourceSettings = {}) {
        pattern = String{sourcePattern};
        flags = sourceFlags;
        settings = sourceSettings;
        REQUIRE_NOTHROW(regex = RegEx::compile(sourcePattern, sourceFlags, sourceSettings));
        REQUIRE(regex != nullptr);
    }

    void requireCompileFail(
        const StringView &sourcePattern, const Flags sourceFlags = {}, const Settings sourceSettings = {}) {
        pattern = String{sourcePattern};
        flags = sourceFlags;
        settings = sourceSettings;
        REQUIRE_THROWS(regex = RegEx::compile(sourcePattern, sourceFlags, sourceSettings));
    }

    void requireMatch(const StringView &subject) {
        text = String{subject};
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->match(subject));
        REQUIRE(lastMatch != nullptr);
    }

    void requireNoMatch(const StringView &subject) {
        text = String{subject};
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->match(subject));
        REQUIRE(lastMatch == nullptr);
    }

    void requireNoMatch(const std::vector<StringView> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoMatch(testCase));
        }
    }

    struct NoCaptureTestCase {
        StringView text;
        InputPosition begin = 0;
        std::optional<StringView> expectedMatch = std::nullopt;
    };
    using NoneCapGroupTestCases = std::vector<NoCaptureTestCase>;

    void requireMatchWithNoneCapGroups(
        const StringView &subject,
        const InputPosition begin = 0,
        const std::optional<StringView> &expectedMatch = std::nullopt) {
        const auto expected = expectedMatch.value_or(subject);
        const auto expectedLine = std::format(
            "00: {:04}-{:04} '{}'",
            begin,
            begin + expected.length().toSizeT(),
            expected.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii));
        WITH_CONTEXT(requireMatch(subject));
        WITH_CONTEXT(requireGroups({expectedLine}));
    }

    void requireMatchWithNoneCapGroups(const std::vector<NoCaptureTestCase> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireMatchWithNoneCapGroups(testCase.text, testCase.begin, testCase.expectedMatch));
        }
    }

    void requireFullMatch(const StringView &subject) {
        text = String{subject};
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->fullMatch(subject));
        REQUIRE(lastMatch != nullptr);
    }

    void requireNoFullMatch(const StringView &subject) {
        text = String{subject};
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->fullMatch(subject));
        REQUIRE(lastMatch == nullptr);
    }

    void requireNoFullMatch(const std::vector<StringView> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoFullMatch(testCase));
        }
    }

    void requireFullMatchWithNoCaptures(const StringView &subject) {
        const auto expectedLine = std::format(
            "00: {:04}-{:04} '{}'",
            0,
            subject.length().toSizeT(),
            subject.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii));
        WITH_CONTEXT(requireFullMatch(subject));
        WITH_CONTEXT(requireGroups({expectedLine}));
    }

    void requireFullMatchWithNoCaptures(const std::vector<StringView> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireFullMatchWithNoCaptures(testCase));
        }
    }

    void requireFindFirst(const StringView &subject) {
        text = String{subject};
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(lastMatch = regex->findFirst(subject));
    }

    void requireNoFindFirst(const StringView &subject) {
        requireFindFirst(subject);
        REQUIRE(lastMatch == nullptr);
    }

    void requireNoFindFirst(const std::vector<StringView> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoFindFirst(testCase));
        }
    }

    void requireFindFirstNoCaptures(
        const StringView &subject,
        const InputPosition begin = 0,
        const std::optional<StringView> &expectedMatch = std::nullopt) {
        const auto expected = expectedMatch.value_or(subject);
        const auto expectedLine = std::format(
            "00: {:04}-{:04} '{}'",
            begin,
            begin + expected.length().toSizeT(),
            expected.toSafeString(el::unit::CpLength{200U}, el::text::SafeStringFlag::OnlyAscii));
        WITH_CONTEXT(requireFindFirst(subject));
        WITH_CONTEXT(requireGroups({expectedLine}));
    }

    void requireFindFirstNoCaptures(const std::vector<NoCaptureTestCase> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireFindFirstNoCaptures(testCase.text, testCase.begin, testCase.expectedMatch));
        }
    }

    void requireFindAll(const StringView &subject) {
        text = String{subject};
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

    void requireNoFindAll(const StringView &subject) {
        text = String{subject};
        REQUIRE(regex != nullptr);
        auto matchCount = 0;
        for ([[maybe_unused]] const auto &match : regex->findAll(subject)) {
            ++matchCount;
        }
        REQUIRE_EQUAL(matchCount, 0);
    }

    void requireNoFindAll(const std::vector<StringView> &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireNoFindAll(testCase));
        }
    }

    void requireCollectAll(const StringView &subject) {
        text = String{subject};
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

    struct ReplaceTestCase {
        StringView text;
        StringView replacementExpression;
        StringView expectedResult;
    };
    using ReplaceTestCases = std::vector<ReplaceTestCase>;

    void requireReplaceAll(const StringView &subject, const StringView &replacementExpression) {
        text = String{subject};
        REQUIRE(regex != nullptr);
        REQUIRE_NOTHROW(textWithReplacements = regex->replaceAll(subject, replacementExpression));
    }

    void requireReplaceAll(const ReplaceTestCases &testCases) {
        for (const auto &testCase : testCases) {
            WITH_CONTEXT(requireReplaceAll(testCase.text, testCase.replacementExpression));
            REQUIRE_EQUAL(textWithReplacements, testCase.expectedResult);
        }
    }

    void requireGroups(const std::vector<std::string> &expectedGroups) {
        WITH_CONTEXT(requireLines(createGroupLines(), expectedGroups));
    }
};
