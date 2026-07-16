// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompilerBase.hpp"

TESTED_TARGETS(Compiler)
TAGS(Compilation)
class CompilerQuantifierTest final : public UNITTEST_SUBCLASS(CompilerBase) {
public:
    // x?
    void testZeroOrOne() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: SPLIT $0003, $0004",
                "$0003: CHAR 'b'",
                "$0004: CHAR 'c'",
                "$0005: MATCH",
            }));
        // lazy
        WITH_CONTEXT(compileAndDisassemble("ab??c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: SPLIT $0004, $0003",
                "$0003: CHAR 'b'",
                "$0004: CHAR 'c'",
                "$0005: MATCH",
            }));
        // possessive (= greedy, atomic)
        WITH_CONTEXT(compileAndDisassemble("ab?+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: SPLIT $0004, $0005",
                "$0004: CHAR 'b'",
                "$0005: STOP ATOMIC 0",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));
    }

    // x*
    void testZeroToMany() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab*c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: SPLIT $0003, $0005",
                "$0003: CHAR 'b'",
                "$0004: JUMP $0001",
                "$0005: CHAR 'c'",
                "$0006: MATCH",
            }));

        // lazy
        WITH_CONTEXT(compileAndDisassemble("ab*?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: SPLIT $0005, $0003",
                "$0003: CHAR 'b'",
                "$0004: JUMP $0001",
                "$0005: CHAR 'c'",
                "$0006: MATCH",
            }));

        // possessive (= greedy, atomic)
        WITH_CONTEXT(compileAndDisassemble("ab*+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: SPLIT $0004, $0006",
                "$0004: CHAR 'b'",
                "$0005: JUMP $0002",
                "$0006: STOP ATOMIC 0",
                "$0007: CHAR 'c'",
                "$0008: MATCH",
            }));
    }

    // x+
    void testOneToMany() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: SPLIT $0001, $0004",
                "$0004: CHAR 'c'",
                "$0005: MATCH",
            }));

        // lazy
        WITH_CONTEXT(compileAndDisassemble("ab+?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: SPLIT $0004, $0001",
                "$0004: CHAR 'c'",
                "$0005: MATCH",
            }));

        // possessive (= greedy, atomic)
        WITH_CONTEXT(compileAndDisassemble("ab++c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: CHAR 'b'",
                "$0003: SPLIT $0002, $0005",
                "$0005: STOP ATOMIC 0",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));
    }

    // {n}
    void testExactly() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab{8}c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: SKIP MAXIMUM 0, 7", // maximum - 1, because the first repetition is already done.
                "$0003: JUMP $0001",
                "$0004: CHAR 'c'",
                "$0005: MATCH",
            }));
        // lazy (should be equal to greedy).
        WITH_CONTEXT(compileAndDisassemble("ab{8}?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: SKIP MAXIMUM 0, 7",
                "$0003: JUMP $0001",
                "$0004: CHAR 'c'",
                "$0005: MATCH",
            }));
        // possessive (atomic groups are expected).
        WITH_CONTEXT(compileAndDisassemble("ab{8}+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: CHAR 'b'",
                "$0003: SKIP MAXIMUM 0, 7",
                "$0004: JUMP $0002",
                "$0005: STOP ATOMIC 0",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));
    }

    // {0,m}
    void testZeroToMaximum() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab{0,8}c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: SPLIT $0003, $0006",
                "$0003: CHAR 'b'",
                "$0004: MAXIMUM 0, 8",
                "$0005: JUMP $0001",
                "$0006: COUNTER 0, 0",
                "$0007: CHAR 'c'",
                "$0008: MATCH",
            }));
        // lazy
        WITH_CONTEXT(compileAndDisassemble("ab{0,8}?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: SPLIT $0006, $0003",
                "$0003: CHAR 'b'",
                "$0004: MAXIMUM 0, 8",
                "$0005: JUMP $0001",
                "$0006: COUNTER 0, 0",
                "$0007: CHAR 'c'",
                "$0008: MATCH",
            }));
        // possessive
        WITH_CONTEXT(compileAndDisassemble("ab{0,8}+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: SPLIT $0004, $0007",
                "$0004: CHAR 'b'",
                "$0005: MAXIMUM 0, 8",
                "$0006: JUMP $0002",
                "$0007: COUNTER 0, 0",
                "$0008: STOP ATOMIC 0",
                "$0009: CHAR 'c'",
                "$000A: MATCH",
            }));
    }

    // {1,m}
    void testOneToMaximum() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab{1,8}c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: MAXIMUM 0, 8",
                "$0003: SPLIT $0001, $0005",
                "$0005: COUNTER 0, 0",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));

        // lazy
        WITH_CONTEXT(compileAndDisassemble("ab{1,8}?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: MAXIMUM 0, 8",
                "$0003: SPLIT $0005, $0001",
                "$0005: COUNTER 0, 0",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));

        // possessive
        WITH_CONTEXT(compileAndDisassemble("ab{1,8}+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: CHAR 'b'",
                "$0003: MAXIMUM 0, 8",
                "$0004: SPLIT $0002, $0006",
                "$0006: COUNTER 0, 0",
                "$0007: STOP ATOMIC 0",
                "$0008: CHAR 'c'",
                "$0009: MATCH",
            }));
    }

    // {n,m}
    void testMinimumToMaximum() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab{3,8}c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: MAXIMUM 0, 8",
                "$0003: SPLIT $0001, $0005",
                "$0005: MINIMUM 0, 3",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));

        // lazy
        WITH_CONTEXT(compileAndDisassemble("ab{3,8}?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: SPLIT $0006, $0003",
                "$0003: CHAR 'b'",
                "$0004: MAXIMUM 0, 8",
                "$0005: JUMP $0001",
                "$0006: MINIMUM 0, 3",
                "$0007: CHAR 'c'",
                "$0008: MATCH",
            }));

        // possessive
        WITH_CONTEXT(compileAndDisassemble("ab{3,8}+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: CHAR 'b'",
                "$0003: MAXIMUM 0, 8",
                "$0004: SPLIT $0002, $0006",
                "$0006: MINIMUM 0, 3",
                "$0007: STOP ATOMIC 0",
                "$0008: CHAR 'c'",
                "$0009: MATCH",
            }));
    }

    // {n,}
    void testMinimumToMany() {
        // greedy
        WITH_CONTEXT(compileAndDisassemble("ab{3,}c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: ADD COUNTER 0, 1",
                "$0003: SPLIT $0001, $0005",
                "$0005: MINIMUM 0, 3",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));

        // lazy
        WITH_CONTEXT(compileAndDisassemble("ab{3,}?c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: CHAR 'b'",
                "$0002: ADD COUNTER 0, 1",
                "$0003: SPLIT $0005, $0001",
                "$0005: MINIMUM 0, 3",
                "$0006: CHAR 'c'",
                "$0007: MATCH",
            }));

        // possessive
        WITH_CONTEXT(compileAndDisassemble("ab{3,}+c"_el));
        WITH_CONTEXT(requireLines(
            lines,
            std::vector<std::string>{
                "$0000: CHAR 'a'",
                "$0001: START ATOMIC 0",
                "$0002: CHAR 'b'",
                "$0003: ADD COUNTER 0, 1",
                "$0004: SPLIT $0002, $0006",
                "$0006: MINIMUM 0, 3",
                "$0007: STOP ATOMIC 0",
                "$0008: CHAR 'c'",
                "$0009: MATCH",
            }));
    }
};
