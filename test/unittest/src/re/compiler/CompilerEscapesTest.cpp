// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompilerBase.hpp"

TESTED_TARGETS(Compiler)
TAGS(Compilation)
class CompilerEscapesTest final : public UNITTEST_SUBCLASS(CompilerBase) {
public:
    void testNullCharacters() {
        Settings settings;
        settings.enableFeature(Feature::AcceptNullInPattern);
        WITH_CONTEXT(compileAndDisassemble("\\x00\\u0000"_el, GroupFlags{}, settings));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR $000000",
                "$0001: CHAR $000000",
                "$0002: MATCH",
            }));
    }

    void testEscapeSequencesForChars() {
        // regular escapes
        WITH_CONTEXT(compileAndDisassemble(R"(\n\r\t\u2192\u{1f600}\a\e\f)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &sequence",
                "$0000: .data $00000A",
                "$0001: .data $00000D",
                "$0002: .data $000009",
                "$0003: .data \"→😀\"",
                "$0005: .data $000007",
                "$0006: .data $00001B",
                "$0007: .data $00000C",
                ".section &program",
                "$0000: SEQUENCE $0000, $08",
                "$0001: MATCH",
            }));
        // legacy negated newline
        WITH_CONTEXT(compileAndDisassemble(R"(\N)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CHAR $00000A",
                "$0001: MATCH",
            }));
    }

    void testEscapeSequencesUnicodeCategories() {
        // Unicode category, outside character class
        WITH_CONTEXT(compileAndDisassemble(R"(\p{C}\p{Nl})"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &Other",
                "$0001: CATEGORY &LetterNumber",
                "$0002: MATCH",
            }));
        // Negated Unicode categories.
        WITH_CONTEXT(compileAndDisassemble(R"(\P{C}\P{Nl})"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &Other",
                "$0001: NOT CATEGORY &LetterNumber",
                "$0002: MATCH",
            }));
    }

    void testEscapeSequencesWordSpaceDigit() {
        // default is Unicode
        WITH_CONTEXT(compileAndDisassemble(R"(\w\s\d)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &WordUnicode",
                "$0001: CATEGORY &SpaceUnicode",
                "$0002: CATEGORY &DigitUnicode",
                "$0003: MATCH",
            }));
        // ascii
        WITH_CONTEXT(compileAndDisassemble(R"(\w\s\d)"_el, GroupFlag::Ascii));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &WordAscii",
                "$0001: CATEGORY &SpaceAscii",
                "$0002: CATEGORY &DigitAscii",
                "$0003: MATCH",
            }));
        // Unicode DotAll
        WITH_CONTEXT(compileAndDisassemble(R"(\w\s\d)"_el, GroupFlag::DotAll));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &WordUnicode",
                "$0001: CATEGORY &SpaceUnicodeDotAll",
                "$0002: CATEGORY &DigitUnicode",
                "$0003: MATCH",
            }));
        // Ascii DotAll
        WITH_CONTEXT(compileAndDisassemble(R"(\w\s\d)"_el, GroupFlags{GroupFlag::DotAll, GroupFlag::Ascii}));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &WordAscii",
                "$0001: CATEGORY &SpaceAsciiDotAll",
                "$0002: CATEGORY &DigitAscii",
                "$0003: MATCH",
            }));
    }

    void testEscapeSequencesWordSpaceDigitNegated() {
        // default is Unicode
        WITH_CONTEXT(compileAndDisassemble(R"(\W\S\D)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &WordUnicode",
                "$0001: NOT CATEGORY &SpaceUnicode",
                "$0002: NOT CATEGORY &DigitUnicode",
                "$0003: MATCH",
            }));
        // ascii
        WITH_CONTEXT(compileAndDisassemble(R"(\W\S\D)"_el, GroupFlag::Ascii));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &WordAscii",
                "$0001: NOT CATEGORY &SpaceAscii",
                "$0002: NOT CATEGORY &DigitAscii",
                "$0003: MATCH",
            }));
        // Unicode DotAll
        WITH_CONTEXT(compileAndDisassemble(R"(\W\S\D)"_el, GroupFlag::DotAll));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &WordUnicode",
                "$0001: NOT CATEGORY &SpaceUnicodeDotAll",
                "$0002: NOT CATEGORY &DigitUnicode",
                "$0003: MATCH",
            }));
        // Ascii DotAll
        WITH_CONTEXT(compileAndDisassemble(R"(\W\S\D)"_el, GroupFlags{GroupFlag::DotAll, GroupFlag::Ascii}));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &WordAscii",
                "$0001: NOT CATEGORY &SpaceAsciiDotAll",
                "$0002: NOT CATEGORY &DigitAscii",
                "$0003: MATCH",
            }));
    }

    void testEscapeHorizontalVerticalSpace() {
        // Unicode
        WITH_CONTEXT(compileAndDisassemble(R"(\h\v)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &HorizontalSpaceUnicode",
                "$0001: CATEGORY &VerticalSpaceUnicode",
                "$0002: MATCH",
            }));
        // Ascii
        WITH_CONTEXT(compileAndDisassemble(R"(\h\v)"_el, GroupFlag::Ascii));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CATEGORY &HorizontalSpaceAscii",
                "$0001: CATEGORY &VerticalSpaceAscii",
                "$0002: MATCH",
            }));
        // Unicode Negated
        WITH_CONTEXT(compileAndDisassemble(R"(\H\V)"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &HorizontalSpaceUnicode",
                "$0001: NOT CATEGORY &VerticalSpaceUnicode",
                "$0002: MATCH",
            }));
        // Ascii Negated
        WITH_CONTEXT(compileAndDisassemble(R"(\H\V)"_el, GroupFlag::Ascii));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: NOT CATEGORY &HorizontalSpaceAscii",
                "$0001: NOT CATEGORY &VerticalSpaceAscii",
                "$0002: MATCH",
            }));
    }
};
