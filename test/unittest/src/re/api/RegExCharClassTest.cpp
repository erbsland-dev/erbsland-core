// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExCharClassBase.hpp"

#include <erbsland/re/StdFormatForRegEx.hpp>

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api CharClasses)
class RegExCharClassTest final : public UNITTEST_SUBCLASS(RegExCharClassBase) {
public:
    /// The test cases for character class matching.
    /// Do not use the character `xyz` in the patterns, as they are used as prefixes in
    /// some of the test cases where only a single character is matched.
    inline static const auto charClassTestCases = CharClassTestCases{
        // --- Literals ---
        CharClassTestCase{.pattern = "[a]"_el, .matches = {U'a'}, .notMatches = {U'b', U'A', U'ä'}},
        CharClassTestCase{.pattern = "[abc]"_el, .matches = {U'a', U'b', U'c'}, .notMatches = {U'd', U'A', U' '}},
        CharClassTestCase{
            .pattern = "[^abc]"_el, .matches = {U'd', U'A', U' ', U'ä'}, .notMatches = {U'a', U'b', U'c'}},

        // --- Ranges ---
        CharClassTestCase{.pattern = "[a-c]"_el, .matches = {U'a', U'b', U'c'}, .notMatches = {U'd', U'A', U'`', U'{'}},
        CharClassTestCase{.pattern = "[0-9]"_el, .matches = {U'0', U'5', U'9'}, .notMatches = {U'a', U'/', U':'}},
        CharClassTestCase{
            .pattern = "[a-c0-1]"_el, .matches = {U'a', U'b', U'c', U'0', U'1'}, .notMatches = {U'd', U'2', U'A'}},
        CharClassTestCase{
            .pattern = "[a-zA-Z]"_el,
            .matches = {U'a', U'm', U'z', U'A', U'M', U'Z'},
            .notMatches = {U'0', U'_', U' '}},

        // --- Special characters in classes ---
        CharClassTestCase{.pattern = "[\\^]"_el, .matches = {U'^'}, .notMatches = {U'a', U'\\'}},
        CharClassTestCase{.pattern = "[\\-]"_el, .matches = {U'-'}, .notMatches = {U'a'}},
        CharClassTestCase{.pattern = "[-a]"_el, .matches = {U'-', U'a'}, .notMatches = {U'b'}},
        CharClassTestCase{.pattern = "[\\-\\]\\[]"_el, .matches = {U'-', U']', U'['}, .notMatches = {U'a', U'\\'}},

        // --- Character types in classes ---
        CharClassTestCase{.pattern = "[\\d]"_el, .matches = {U'0', U'9', U'٠'}, .notMatches = {U'a', U' '}},
        CharClassTestCase{.pattern = "[\\s]"_el, .matches = {U' ', U'\t'}, .notMatches = {U'a', U'0', U'\n', U'\r'}},
        CharClassTestCase{.pattern = "[\\w]"_el, .matches = {U'a', U'0', U'_', U'ä'}, .notMatches = {U' ', U'.'}},
        CharClassTestCase{.pattern = "[\\h]"_el, .matches = {U' ', U'\t'}, .notMatches = {U'\n', U'a'}},
        CharClassTestCase{.pattern = "[\\v]"_el, .matches = {U'\n', U'\r'}, .notMatches = {U' ', U'a'}},
        CharClassTestCase{.pattern = "[\\p{L}]"_el, .matches = {U'a', U'ä', U'Λ'}, .notMatches = {U'0', U' '}},
        CharClassTestCase{.pattern = "[a\\db]"_el, .matches = {U'a', U'5', U'b'}, .notMatches = {U'c', U' '}},

        // --- Character type escapes ---
        CharClassTestCase{.pattern = "."_el, .matches = {U'a', U'0', U' ', U'ä', U'💩'}, .notMatches = {U'\n'}},
        CharClassTestCase{.pattern = "\\d"_el, .matches = {U'0', U'9', U'٠'}, .notMatches = {U'a', U' '}},
        CharClassTestCase{.pattern = "\\D"_el, .matches = {U'a', U' ', U'ä'}, .notMatches = {U'0', U'9', U'٠'}},
        CharClassTestCase{
            .pattern = "\\s"_el, .matches = {U' ', U'\t', U' '}, .notMatches = {U'a', U'0', U'\n', U'\r'}},
        CharClassTestCase{
            .pattern = "\\S"_el, .matches = {U'a', U'0', U'ä', U'\n', U'\r'}, .notMatches = {U' ', U'\t'}},
        CharClassTestCase{
            .pattern = "\\w"_el, .matches = {U'a', U'Z', U'0', U'9', U'_', U'ä'}, .notMatches = {U' ', U'.', U'💩'}},
        CharClassTestCase{.pattern = "\\W"_el, .matches = {U' ', U'.', U'💩'}, .notMatches = {U'a', U'0', U'_'}},
        CharClassTestCase{.pattern = "\\h"_el, .matches = {U' ', U'\t', U' '}, .notMatches = {U'\n', U'a'}},
        CharClassTestCase{.pattern = "\\H"_el, .matches = {U'\n', U'a'}, .notMatches = {U' ', U' '}},
        CharClassTestCase{.pattern = "\\v"_el, .matches = {U'\n', U'\r', U'\f'}, .notMatches = {U' ', U'a'}},
        CharClassTestCase{.pattern = "\\V"_el, .matches = {U' ', U'a'}, .notMatches = {U'\n', U'\r'}},

        // --- Unicode categories ---
        CharClassTestCase{
            .pattern = "\\p{L}"_el, .matches = {U'a', U'A', U'ä', U'Λ'}, .notMatches = {U'0', U' ', U'.'}},
        CharClassTestCase{.pattern = "\\p{Lu}"_el, .matches = {U'A', U'Λ'}, .notMatches = {U'a', U'λ', U'0'}},
        CharClassTestCase{
            .pattern = "\\p{Uppercase_Letter}"_el, .matches = {U'A', U'Λ'}, .notMatches = {U'a', U'λ', U'0'}},
        CharClassTestCase{
            .pattern = "\\p{lowercase_letter}"_el, .matches = {U'a', U'λ'}, .notMatches = {U'A', U'Λ', U'0'}},
        CharClassTestCase{.pattern = "\\P{L}"_el, .matches = {U'0', U' ', U'.', U'💩'}, .notMatches = {U'a', U'ä'}},
        CharClassTestCase{.pattern = "\\p{Nd}"_el, .matches = {U'0', U'9', U'٠'}, .notMatches = {U'a', U' '}},
        CharClassTestCase{.pattern = "\\p{Z}"_el, .matches = {U' ', U' ', U' '}, .notMatches = {U'a', U'\n'}},

        // --- POSIX classes ---
        CharClassTestCase{.pattern = "[[:digit:]]"_el, .matches = {U'0', U'9', U'٠'}, .notMatches = {U'a', U' '}},
        CharClassTestCase{.pattern = "[[:alpha:]]"_el, .matches = {U'a', U'A', U'ä'}, .notMatches = {U'0', U' ', U'_'}},
        CharClassTestCase{.pattern = "[[:alnum:]]"_el, .matches = {U'a', U'0', U'ä', U'٠'}, .notMatches = {U' ', U'_'}},
        CharClassTestCase{.pattern = "[[:space:]]"_el, .matches = {U' ', U'\t', U'\n'}, .notMatches = {U'a'}},
        CharClassTestCase{
            .pattern = "[[:xdigit:]]"_el, .matches = {U'0', U'9', U'a', U'f', U'A', U'F'}, .notMatches = {U'g', U' '}},
        CharClassTestCase{.pattern = "[[:word:]]"_el, .matches = {U'a', U'0', U'_'}, .notMatches = {U' ', U'.'}},
        CharClassTestCase{.pattern = "[[:^digit:]]"_el, .matches = {U'a', U' ', U'.'}, .notMatches = {U'0', U'9'}},
        CharClassTestCase{.pattern = "[[:alpha:][:digit:]]"_el, .matches = {U'a', U'0'}, .notMatches = {U' ', U'_'}},

        // --- Quoted literals ---
        CharClassTestCase{.pattern = "[\\Qabc\\E]"_el, .matches = {U'a', U'b', U'c'}, .notMatches = {U'd', U'\\'}},
        CharClassTestCase{.pattern = "[\\Q.-[]\\E]"_el, .matches = {U'.', U'-', U'[', U']'}, .notMatches = {U'a'}},

        // --- Flags ---
        CharClassTestCase{
            .pattern = "(?i:[a-c])"_el, .matches = {U'a', U'A', U'b', U'B', U'c', U'C'}, .notMatches = {U'd', U'D'}},
        CharClassTestCase{
            .pattern = "(?i:[^a-c])"_el, .matches = {U'd', U'D', U'0'}, .notMatches = {U'a', U'A', U'b', U'B'}},
        CharClassTestCase{.pattern = "(?i:\\p{Lu})"_el, .matches = {U'A', U'Λ'}, .notMatches = {U'a', U'λ'}},
        CharClassTestCase{.pattern = "(?a:\\d)"_el, .matches = {U'0', U'9'}, .notMatches = {U'٠', U'a'}},
        CharClassTestCase{.pattern = "(?a:\\w)"_el, .matches = {U'a', U'A', U'0', U'_'}, .notMatches = {U'ä', U'ö'}},
        CharClassTestCase{.pattern = "(?a:[[:digit:]])"_el, .matches = {U'0', U'9'}, .notMatches = {U'٠'}},
        CharClassTestCase{.pattern = "(?s:.)"_el, .matches = {U'a', U'\n'}, .notMatches = {}},
        CharClassTestCase{.pattern = "(?s:\\s)"_el, .matches = {U' ', U'\n'}, .notMatches = {U'a'}},
    };

    TESTED_TARGETS(match)
    void testMatch() {
        // ■ = inserts the test pattern
        // ● = inserts the tested character
        // ⟪ ... ⟫ = marks an expected match in the text.
        const auto testPatterns = PatternTestCases{
            {"■"_el, {"⟪●⟫"_el, "⟪●⟫xyz"_el}},
            {"■xyz"_el, {"⟪●xyz⟫"_el}},
            {"abc■xyz"_el, {"⟪abc●xyz⟫"_el}},
            {"abc■"_el, {"⟪abc●⟫"_el, "⟪abc●⟫xyz"_el}},
            {"►■◄"_el, {"⟪►●◄⟫"_el, "⟪►●◄⟫xyz"_el}},
        };
        requireCharClassMatch(charClassTestCases, testPatterns, [this](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            REQUIRE_NOTHROW(lastMatch = regex->match(preparedPattern.text));
            REQUIRE(lastMatch != nullptr); // expect a match.
            // validate the matching groups.
            REQUIRE_EQUAL(preparedPattern.expectedGroupLines.size(), 1);
            const auto &firstMatchLocation = preparedPattern.expectedMatchLocations[0];
            auto expectedGroups = std::vector<std::string>{preparedPattern.expectedGroupLines[0]};
            WITH_CONTEXT(requireGroups(expectedGroups));
        });
        requireCharClassNotMatch(charClassTestCases, testPatterns, [&](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            REQUIRE_NOTHROW(lastMatch = regex->match(preparedPattern.text));
            REQUIRE(lastMatch == nullptr); // expect no match.
        });
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        // ■ = inserts the test pattern
        // ● = inserts the tested character
        // ⟪ ... ⟫ = marks an expected match in the text.
        const auto testPatterns = PatternTestCases{
            {"■"_el, {"⟪●⟫"_el}},
            {"■xyz"_el, {"⟪●xyz⟫"_el}},
            {"abc■xyz"_el, {"⟪abc●xyz⟫"_el}},
            {"►■◄"_el, {"⟪►●◄⟫"_el}},
        };
        requireCharClassMatch(charClassTestCases, testPatterns, [this](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            REQUIRE_NOTHROW(lastMatch = regex->fullMatch(preparedPattern.text));
            REQUIRE(lastMatch != nullptr); // expect a match.
            // validate the matching groups.
            REQUIRE_EQUAL(preparedPattern.expectedGroupLines.size(), 1);
            const auto &firstMatchLocation = preparedPattern.expectedMatchLocations[0];
            auto expectedGroups = std::vector<std::string>{preparedPattern.expectedGroupLines[0]};
            WITH_CONTEXT(requireGroups(expectedGroups));
        });
        requireCharClassNotMatch(charClassTestCases, testPatterns, [&](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            REQUIRE_NOTHROW(lastMatch = regex->fullMatch(preparedPattern.text));
            REQUIRE(lastMatch == nullptr); // expect no match.
        });
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        // ■ = inserts the test pattern
        // ● = inserts the tested character
        // ⟪ ... ⟫ = marks an expected match in the text.
        const auto testPatterns = PatternTestCases{
            {"■"_el, {"⟪●⟫"_el}},
            {"■xyz"_el, {"⟪●xyz⟫"_el}},
            {"abc■xyz"_el, {"⟪abc●xyz⟫"_el}},
            {"►■◄"_el, {"⟪►●◄⟫"_el}},
        };
        requireCharClassMatch(charClassTestCases, testPatterns, [this](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            REQUIRE_NOTHROW(lastMatch = regex->findFirst(preparedPattern.text));
            REQUIRE(lastMatch != nullptr); // expect a match.
            // validate the matching groups.
            REQUIRE_EQUAL(preparedPattern.expectedGroupLines.size(), 1);
            const auto &firstMatchLocation = preparedPattern.expectedMatchLocations[0];
            auto expectedGroups = std::vector<std::string>{preparedPattern.expectedGroupLines[0]};
            WITH_CONTEXT(requireGroups(expectedGroups));
        });
        requireCharClassNotMatch(charClassTestCases, testPatterns, [&](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            REQUIRE_NOTHROW(lastMatch = regex->findFirst(preparedPattern.text));
            REQUIRE(lastMatch == nullptr); // expect no match.
        });
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        // ■ = inserts the test pattern
        // ● = inserts the tested character
        // ⟪ ... ⟫ = marks an expected match in the text.
        const auto testPatterns = PatternTestCases{
            {"■"_el, {"⟪●⟫"_el, "⟪●⟫⟪●⟫⟪●⟫"_el}},
            {"■xyz"_el, {"⟪●xyz⟫"_el}},
            {"abc■xyz"_el, {"⟪abc●xyz⟫"_el}},
            {"►■◄"_el, {"⟪►●◄⟫"_el}},
        };
        requireCharClassMatch(charClassTestCases, testPatterns, [this](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            std::size_t index = 0;
            for (const auto &match : regex->findAll(preparedPattern.text)) {
                lastMatch = match;
                REQUIRE(index < preparedPattern.expectedGroupLines.size());
                const auto &firstMatchLocation = preparedPattern.expectedMatchLocations[index];
                auto expectedGroups = std::vector<std::string>{preparedPattern.expectedGroupLines[index]};
                WITH_CONTEXT(requireGroups(expectedGroups));
                index += 1;
            }
        });
        requireCharClassNotMatch(charClassTestCases, testPatterns, [&](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            std::size_t index = 0;
            for (const auto &match : regex->findAll(preparedPattern.text)) {
                lastMatch = match;
                index += 1;
            }
            REQUIRE_EQUAL(index, 0);
        });
    }

    void testCollectAll() {
        // ■ = inserts the test pattern
        // ● = inserts the tested character
        // ⟪ ... ⟫ = marks an expected match in the text.
        const auto testPatterns = PatternTestCases{
            {"■"_el, {"⟪●⟫"_el, "⟪●⟫⟪●⟫⟪●⟫"_el}},
            {"■xyz"_el, {"⟪●xyz⟫"_el}},
            {"abc■xyz"_el, {"⟪abc●xyz⟫"_el}},
            {"►■◄"_el, {"⟪►●◄⟫"_el}},
        };
        requireCharClassMatch(charClassTestCases, testPatterns, [this](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            std::size_t index = 0;
            for (const auto &match : regex->collectAll(preparedPattern.text)) {
                lastMatch = match;
                REQUIRE(index < preparedPattern.expectedGroupLines.size());
                const auto &firstMatchLocation = preparedPattern.expectedMatchLocations[index];
                auto expectedGroups = std::vector<std::string>{preparedPattern.expectedGroupLines[index]};
                WITH_CONTEXT(requireGroups(expectedGroups));
                index += 1;
            }
        });
        requireCharClassNotMatch(charClassTestCases, testPatterns, [&](const PreparedPattern &preparedPattern) -> void {
            REQUIRE_NOTHROW(regex = RegEx::compile(preparedPattern.pattern));
            std::size_t index = 0;
            for (const auto &match : regex->collectAll(preparedPattern.text)) {
                lastMatch = match;
                index += 1;
            }
            REQUIRE_EQUAL(index, 0);
        });
    }
};
