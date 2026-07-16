// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompilerBase.hpp"

TESTED_TARGETS(Compiler)
TAGS(Compilation)
class CompilerBasicTest final : public UNITTEST_SUBCLASS(CompilerBase) {
public:
    void testEmptyPattern() {
        Settings settings;
        settings.enableFeature(Feature::EmptyGroups);
        WITH_CONTEXT(compileAndDisassemble(""_el, {}, settings));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: MATCH",
            }));
    }

    void testShortCharSequence() {
        WITH_CONTEXT(compileAndDisassemble("a"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: MATCH",
            }));

        WITH_CONTEXT(compileAndDisassemble("abc"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: CHAR 'c'",
                "$0003: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("ABC"_el, GroupFlag::IgnoreCase));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CI CHAR 'a'",
                "$0001: CI CHAR 'b'",
                "$0002: CI CHAR 'c'",
                "$0003: MATCH",
            }));
    }

    void testAnchor() {
        WITH_CONTEXT(compileAndDisassemble("^abc$"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: ANCHOR &Start",
                "$0001: CHAR 'a'",
                "$0002: CHAR 'b'",
                "$0003: CHAR 'c'",
                "$0004: ANCHOR &End",
                "$0005: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("^abc$"_el, GroupFlag::Multiline));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: ANCHOR &LineStart",
                "$0001: CHAR 'a'",
                "$0002: CHAR 'b'",
                "$0003: CHAR 'c'",
                "$0004: ANCHOR &LineEnd",
                "$0005: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"(\bxyz\b)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: ANCHOR &UnicodeWordBoundary",
                "$0001: CHAR 'x'",
                "$0002: CHAR 'y'",
                "$0003: CHAR 'z'",
                "$0004: ANCHOR &UnicodeWordBoundary",
                "$0005: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"(\bxyz\b)"_el, GroupFlag::Ascii));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: ANCHOR &AsciiWordBoundary",
                "$0001: CHAR 'x'",
                "$0002: CHAR 'y'",
                "$0003: CHAR 'z'",
                "$0004: ANCHOR &AsciiWordBoundary",
                "$0005: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"(\Bxyz\B)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: ANCHOR &NonUnicodeWordBoundary",
                "$0001: CHAR 'x'",
                "$0002: CHAR 'y'",
                "$0003: CHAR 'z'",
                "$0004: ANCHOR &NonUnicodeWordBoundary",
                "$0005: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble(R"(\Bxyz\B)"_el, GroupFlag::Ascii));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: ANCHOR &NonAsciiWordBoundary",
                "$0001: CHAR 'x'",
                "$0002: CHAR 'y'",
                "$0003: CHAR 'z'",
                "$0004: ANCHOR &NonAsciiWordBoundary",
                "$0005: MATCH",
            }));
    }

    void testAny() {
        WITH_CONTEXT(compileAndDisassemble("."_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CHAR $00000A",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("."_el, GroupFlag::DotAll));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: ANY",
                "$0001: MATCH",
            }));
    }
};
