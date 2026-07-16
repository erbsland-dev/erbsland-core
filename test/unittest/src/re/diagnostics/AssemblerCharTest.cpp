// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

TESTED_TARGETS(Assembler)
TAGS(Diagnostics)
class AssemblerCharTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testClassLabelOnClassLine() {
        // A label placed on a `.class` line must refer to the newly started class.
        // This is especially important when the previous class is still open and will be closed by `.class`.
        WITH_CONTEXT(requireCompile({
            ".section &class",
            "c1: .class",
            "    .data 'a'",
            "c2: .class",
            "    .data 'b'",
            "c3: .class",
            "    .data 'c'",
            ".section &program",
            "CLASS %c1",
            "CLASS %c2",
            "CLASS %c3",
            "MATCH",
        }));

        WITH_CONTEXT(requireClass(0x0000));
        WITH_CONTEXT(requireClass(0x0001));
        WITH_CONTEXT(requireClass(0x0002));
        WITH_CONTEXT(requireMatch());
    }

    void testCharOperation() {
        WITH_CONTEXT(requireCompile({
            "CHAR 97",  // decimal 'a'
            "CHAR 'Z'", // char format
            "CHAR '\\\''",
            "CHAR '\\\\'",
            "CHAR '😄'",
            "CI CHAR 'a'",
            "NOT CHAR 'a'",
            "NOT CI CHAR 'a'",
        }));
        WITH_CONTEXT(requireChar(U'a'));
        WITH_CONTEXT(requireChar(U'Z'));
        WITH_CONTEXT(requireChar(U'\''));
        WITH_CONTEXT(requireChar(U'\\'));
        WITH_CONTEXT(requireChar(U'😄'));
        WITH_CONTEXT(requireCiChar(U'a'));
        WITH_CONTEXT(requireNotChar(U'a'));
        WITH_CONTEXT(requireNotCiChar(U'a'));

        WITH_CONTEXT(requireCompilerError({"CHAR '"}));
        WITH_CONTEXT(requireCompilerError({"CHAR '\\"}));
        WITH_CONTEXT(requireCompilerError({"CHAR '\\'"}));
        WITH_CONTEXT(requireCompilerError({"CHAR '\\\\"}));
        WITH_CONTEXT(requireCompilerError({"CHAR 'ab'"}));
        WITH_CONTEXT(requireCompilerError({"CHAR 'a\\\''"}));
        WITH_CONTEXT(requireCompilerError({"CHAR 'a''"}));
        WITH_CONTEXT(
            requireCompilerError({"CHAR 1114112"}), "Character code is out of the valid Unicode range"); // 0x110000
        WITH_CONTEXT(requireCompilerError({"CHAR 97x"}), "integer format");
        WITH_CONTEXT(requireCompilerError({"CHAR false"}));
    }

    void testSequenceOperation() {
        WITH_CONTEXT(requireCompile({
            "SEQUENCE 10, 5",
            "CI SEQUENCE 11, 6",
        }));
        WITH_CONTEXT(requireSequence(10, 5));
        WITH_CONTEXT(requireCiSequence(11, 6));

        // Errors
        WITH_CONTEXT(requireCompilerError({"SEQUENCE 70000, 1"}), "out of range");
        WITH_CONTEXT(requireCompilerError({"SEQUENCE 1, 0"}), "out of range");
        WITH_CONTEXT(requireCompilerError({"SEQUENCE 1, 256"}), "out of range");
        WITH_CONTEXT(requireCompilerError({"SEQUENCE 1x, 1"}), "integer format");
        WITH_CONTEXT(requireCompilerError({"SEQUENCE 1, 1x"}), "integer format");
        WITH_CONTEXT(requireCompilerError({"SEQUENCE &abc, 1"}));
        WITH_CONTEXT(requireCompilerError({"SEQUENCE 1, &abc"}));
    }

    void testClassOperation() {
        WITH_CONTEXT(requireCompile({"CLASS $0123", "NOT CLASS 65533", "CI CLASS 1", "NOT CI CLASS 12"}));
        WITH_CONTEXT(requireClass(0x0123U));
        WITH_CONTEXT(requireNotClass(65533U));
        WITH_CONTEXT(requireCiClass(1U));
        WITH_CONTEXT(requireNotCiClass(12U));

        WITH_CONTEXT(requireCompilerError({"CLASS 65535"}));
        WITH_CONTEXT(requireCompilerError({"CLASS 10000000"}));
        WITH_CONTEXT(requireCompilerError({"CLASS &abc"}));
        WITH_CONTEXT(requireCompilerError({"CLASS false"}));
    }
};
