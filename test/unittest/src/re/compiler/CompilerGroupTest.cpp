// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompilerBase.hpp"

TESTED_TARGETS(Compiler)
TAGS(Compilation)
class CompilerGroupTest final : public UNITTEST_SUBCLASS(CompilerBase) {
public:
    void testFoldedGroup() {
        WITH_CONTEXT(compileAndDisassemble("(?:abc)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: CHAR 'c'",
                "$0003: MATCH",
            }));
    }

    void testLiteralAlternativeForms() {
        const auto expected = std::vector<std::string>{
            ".section &sequence",
            "$0000: .data \"hello\"",
            "$0005: .data \"world\"",
            ".section &program",
            "$0000: SPLIT $0002, $0004",
            "$0002: SEQUENCE $0000, $05",
            "$0003: JUMP $0005",
            "$0004: SEQUENCE $0005, $05",
            "$0005: MATCH",
        };

        WITH_CONTEXT(compileAndDisassemble("hello|world"_el));
        WITH_CONTEXT(requireLines(lines, expected));
        WITH_CONTEXT(compileAndDisassemble("(?:hello|world)"_el));
        WITH_CONTEXT(requireLines(lines, expected));
    }

    void testLiteralFastPathRespectsCompilerLimits() {
        auto settings = Settings{};
        settings.setMaximumPatternLength(el::unit::CpLength{5U});
        REQUIRE_THROWS(compileAndDisassemble("hello"_el, {}, settings));

        settings = Settings{};
        settings.setMaximumAlternativeCount(1U);
        REQUIRE_THROWS(compileAndDisassemble("hello|world"_el, {}, settings));

        REQUIRE_THROWS(compileAndDisassemble("hello"_el, GroupFlag::Atomic));
    }

    void testRootAlternationTwo() {
        WITH_CONTEXT(compileAndDisassemble("a|b"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: SPLIT $0002, $0004",
                "$0002: CHAR 'a'",
                "$0003: JUMP $0005",
                "$0004: CHAR 'b'",
                "$0005: MATCH",
            }));
    }

    void testRootAlternationThree() {
        WITH_CONTEXT(compileAndDisassemble("a|b|c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: SPLIT $0004, $0002",
                "$0002: SPLIT $0006, $0008",
                "$0004: CHAR 'a'",
                "$0005: JUMP $0009",
                "$0006: CHAR 'b'",
                "$0007: JUMP $0009",
                "$0008: CHAR 'c'",
                "$0009: MATCH",
            }));
    }

    void testRootAlternationFour() {
        WITH_CONTEXT(compileAndDisassemble("a|b|c|d"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: SPLIT $0006, $0002",
                "$0002: SPLIT $0008, $0004",
                "$0004: SPLIT $000A, $000C",
                "$0006: CHAR 'a'",
                "$0007: JUMP $000D",
                "$0008: CHAR 'b'",
                "$0009: JUMP $000D",
                "$000A: CHAR 'c'",
                "$000B: JUMP $000D",
                "$000C: CHAR 'd'",
                "$000D: MATCH",
            }));
    }

    void testFoldedAlternationInGroup() {
        WITH_CONTEXT(compileAndDisassemble("(?:a|b|c)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: SPLIT $0004, $0002",
                "$0002: SPLIT $0006, $0008",
                "$0004: CHAR 'a'",
                "$0005: JUMP $0009",
                "$0006: CHAR 'b'",
                "$0007: JUMP $0009",
                "$0008: CHAR 'c'",
                "$0009: MATCH",
            }));
    }

    void testAlternationInGroupTwo() {
        WITH_CONTEXT(compileAndDisassemble("x(?:a|b)y"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: SPLIT $0003, $0005",
                "$0003: CHAR 'a'",
                "$0004: JUMP $0006",
                "$0005: CHAR 'b'",
                "$0006: CHAR 'y'",
                "$0007: MATCH",
            }));
    }

    void testAlternationInGroupThree() {
        WITH_CONTEXT(compileAndDisassemble("x(?:a|b|c)y"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: SPLIT $0005, $0003",
                "$0003: SPLIT $0007, $0009",
                "$0005: CHAR 'a'",
                "$0006: JUMP $000A",
                "$0007: CHAR 'b'",
                "$0008: JUMP $000A",
                "$0009: CHAR 'c'",
                "$000A: CHAR 'y'",
                "$000B: MATCH",
            }));
    }

    void testCaptureGroup() {
        WITH_CONTEXT(compileAndDisassemble("x(.*)y"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: START CAPTURE 0",
                "$0002: SPLIT $0004, $0006",
                "$0004: NOT CHAR $00000A",
                "$0005: JUMP $0002",
                "$0006: STOP CAPTURE 0",
                "$0007: CHAR 'y'",
                "$0008: MATCH",
            }));
    }

    void testCaptureGroupSequence() {
        WITH_CONTEXT(compileAndDisassemble("x(a|b)(c|d)y"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: START CAPTURE 0",
                "$0002: SPLIT $0004, $0006",
                "$0004: CHAR 'a'",
                "$0005: JUMP $0007",
                "$0006: CHAR 'b'",
                "$0007: STOP CAPTURE 0",
                "$0008: START CAPTURE 1",
                "$0009: SPLIT $000B, $000D",
                "$000B: CHAR 'c'",
                "$000C: JUMP $000E",
                "$000D: CHAR 'd'",
                "$000E: STOP CAPTURE 1",
                "$000F: CHAR 'y'",
                "$0010: MATCH",
            }));
    }

    void testNestedCaptureGroups() {
        WITH_CONTEXT(compileAndDisassemble("x(a|(b|c)|(d|e))y"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: START CAPTURE 0",
                "$0002: SPLIT $0006, $0004",
                "$0004: SPLIT $0008, $0010",
                "$0006: CHAR 'a'",
                "$0007: JUMP $0017",
                "$0008: START CAPTURE 1",
                "$0009: SPLIT $000B, $000D",
                "$000B: CHAR 'b'",
                "$000C: JUMP $000E",
                "$000D: CHAR 'c'",
                "$000E: STOP CAPTURE 1",
                "$000F: JUMP $0017",
                "$0010: START CAPTURE 2",
                "$0011: SPLIT $0013, $0015",
                "$0013: CHAR 'd'",
                "$0014: JUMP $0016",
                "$0015: CHAR 'e'",
                "$0016: STOP CAPTURE 2",
                "$0017: STOP CAPTURE 0",
                "$0018: CHAR 'y'",
                "$0019: MATCH",
            }));
    }

    void testGroupWithQuantifier() {
        WITH_CONTEXT(compileAndDisassemble("(?:abc)+"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: CHAR 'c'",
                "$0003: SPLIT $0000, $0005",
                "$0005: MATCH",
            }));
    }

    void testAtomicGroups() {
        WITH_CONTEXT(compileAndDisassemble("x(?>y(?:abc)*)z"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: START ATOMIC 0",
                "$0002: CHAR 'y'",
                "$0003: SPLIT $0005, $0009",
                "$0005: CHAR 'a'",
                "$0006: CHAR 'b'",
                "$0007: CHAR 'c'",
                "$0008: JUMP $0003",
                "$0009: STOP ATOMIC 0",
                "$000A: CHAR 'z'",
                "$000B: MATCH",
            }));
    }

    void testLocalFlagChanges() {
        WITH_CONTEXT(compileAndDisassemble("xa(?i:a)y"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: CHAR 'a'",
                "$0002: CI CHAR 'a'",
                "$0003: CHAR 'y'",
                "$0004: MATCH",
            }));
    }
};
