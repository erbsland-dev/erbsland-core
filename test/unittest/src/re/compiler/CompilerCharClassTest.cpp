// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompilerBase.hpp"

TESTED_TARGETS(Compiler)
TAGS(Compilation)
class CompilerCharClassTest final : public UNITTEST_SUBCLASS(CompilerBase) {
public:
    void testSingleCharClass() {
        WITH_CONTEXT(compileAndDisassemble("[x]"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'x'",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("[^x]"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CHAR 'x'",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("[x]"_el, GroupFlag::IgnoreCase));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CI CHAR 'x'",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("[^x]"_el, GroupFlag::IgnoreCase));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CI CHAR 'x'",
                "$0001: MATCH",
            }));
    }

    void testClassWithSingleUnicodeCategories() {
        WITH_CONTEXT(compileAndDisassemble(R"([\p{Lu}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &UppercaseLetter",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"([^\p{Lu}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &UppercaseLetter",
                "$0001: MATCH",
            }));

        // If categories overlap, they are normalized to remove duplicates and subcategories.
        WITH_CONTEXT(compileAndDisassemble(R"([^\p{Lu}\p{Lu}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &UppercaseLetter",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"([^\p{L}\p{Lu}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &Letter",
                "$0001: MATCH",
            }));
    }

    void testClassWithMultipleUnicodeCategories() {
        // Classes with only categories aren't converted into character classes.
        // Instead, they are modeled like a group of category alternatives.
        WITH_CONTEXT(compileAndDisassemble(R"([\p{Lu}\p{Ll}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: SPLIT $0002, $0004",
                "$0002: CATEGORY &LowercaseLetter",
                "$0003: JUMP $0005",
                "$0004: CATEGORY &UppercaseLetter",
                "$0005: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"([\p{Lu}\p{Ll}\p{S}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: SPLIT $0004, $0002",
                "$0002: SPLIT $0006, $0008",
                "$0004: CATEGORY &LowercaseLetter",
                "$0005: JUMP $0009",
                "$0006: CATEGORY &UppercaseLetter",
                "$0007: JUMP $0009",
                "$0008: CATEGORY &Symbol",
                "$0009: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"([\p{Lu}\p{Ll}\p{S}\p{Z}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: SPLIT $0006, $0002",
                "$0002: SPLIT $0008, $0004",
                "$0004: SPLIT $000A, $000C",
                "$0006: CATEGORY &LowercaseLetter",
                "$0007: JUMP $000D",
                "$0008: CATEGORY &UppercaseLetter",
                "$0009: JUMP $000D",
                "$000A: CATEGORY &Symbol",
                "$000B: JUMP $000D",
                "$000C: CATEGORY &Separator",
                "$000D: MATCH",
            }));
    }

    void testClassWithNegatedUnicodeCategories() {
        WITH_CONTEXT(compileAndDisassemble(R"([^\p{Lu}\p{Ll}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &LowercaseLetter",
                "$0001: NOT ASSERT CATEGORY &UppercaseLetter",
                "$0002: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"([^\p{Lu}\p{Ll}\p{Z}])"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &LowercaseLetter",
                "$0001: NOT ASSERT CATEGORY &UppercaseLetter",
                "$0002: NOT ASSERT CATEGORY &Separator",
                "$0003: MATCH",
            }));
    }

    void testClassWithSingleRange() {
        WITH_CONTEXT(compileAndDisassemble("[a-z]"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &class",
                "$0000: .class",
                "$0000: .data $000061-$00007A",
                ".section &program",
                "$0000: CLASS $0000",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("[^a-z]"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &class",
                "$0000: .class",
                "$0000: .data $000061-$00007A",
                ".section &program",
                "$0000: NOT CLASS $0000",
                "$0001: MATCH",
            }));
    }
};
