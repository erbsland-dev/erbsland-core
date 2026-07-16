// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompilerBase.hpp"

TESTED_TARGETS(Compiler)
TAGS(Compilation) class CodeGeneratorSequenceTest final : public UNITTEST_SUBCLASS(CompilerBase) {
public:
    void testLongCharSequence() {
        WITH_CONTEXT(compileAndDisassemble("ElRE"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &sequence",
                "$0000: .data \"ElRE\"",
                ".section &program",
                "$0000: SEQUENCE $0000, $04",
                "$0001: MATCH",
            }));
        WITH_CONTEXT(compileAndDisassemble("E⒭bsland"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &sequence",
                "$0000: .data \"E⒭bsland\"",
                ".section &program",
                "$0000: SEQUENCE $0000, $08",
                "$0001: MATCH",
            }));
    }

    void testVeryLongSequence() {
        const auto veryLongPattern = String::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{300U});

        // This should not throw and should generate two SEQUENCE operations.
        WITH_CONTEXT(compileAndDisassemble(veryLongPattern));

        // 255 + 45 = 300
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &sequence",
                "$0000: .data \"*\"",
                "$00FF: .data \"*\"",
                ".section &program",
                "$0000: SEQUENCE $0000, $ff",
                "$0001: SEQUENCE $00ff, $2d",
                "$0002: MATCH",
            }));
    }

    void testVeryLongCiSequence() {
        const auto veryLongPattern = String::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{300U});

        // This should not throw and should generate two CI SEQUENCE operations.
        WITH_CONTEXT(compileAndDisassemble(veryLongPattern, GroupFlag::IgnoreCase));

        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                ".section &sequence",
                "$0000: .data \"*\"",
                ".section &program",
                "$0000: CI SEQUENCE $0000, $ff",
                "$0001: CI SEQUENCE $00ff, $2d",
                "$0002: MATCH",
            }));
    }
};
