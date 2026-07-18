// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

TESTED_TARGETS(Assembler)
TAGS(Diagnostics)
class AssemblerBasicTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testNone() {
        WITH_CONTEXT(requireCompile({"None"}));
        WITH_CONTEXT(requireNone());
    }

    void testWhitespaceAndComments() {
        // Empty and comment-only lines are allowed; label-only is not (invalid op)
        WITH_CONTEXT(requireCompile({
            "   ",
            "; just a comment",
            "Start:\tNone" // NOTE: label + op separated by plain tab after colon is fine (trimmed)
        }));
        WITH_CONTEXT(requireNone());

        // Spacing can be any combination of spaces and/or tabs between op and first argument
        WITH_CONTEXT(requireCompile({
            "JUMP    0",
            "JUMP\t\t0",
            "JUMP \t 0",
            "SPLIT \t  1, \t2",
        }));
        WITH_CONTEXT(requireJump(0x0000));
        WITH_CONTEXT(requireJump(0x0000));
        WITH_CONTEXT(requireJump(0x0000));
        WITH_CONTEXT(requireSplit(0x0001, 0x0002));

        WITH_CONTEXT(requireCompile({
            "JUMP    0\t",
            "JUMP\t\t0",
            "JUMP \t 0 ",
            "SPLIT \t  1, \t2",
            "SPLIT 1  \t,2  \t",
            "JUMP 0; a comment",
            "JUMP 0 \t  ;a comment   \t \t",
        }));
        WITH_CONTEXT(requireJump(0x0000));
        WITH_CONTEXT(requireJump(0x0000));
        WITH_CONTEXT(requireJump(0x0000));
        WITH_CONTEXT(requireSplit(0x0001, 0x0002));
        WITH_CONTEXT(requireSplit(0x0001, 0x0002));
        WITH_CONTEXT(requireJump(0x0000));
        WITH_CONTEXT(requireJump(0x0000));
    }

    void testGroupNames() {
        WITH_CONTEXT(requireCompile({
            ".groups 2",
            ".group 1 \"first\"",
            "None",
        }));
        REQUIRE_EQUAL(engineData->captureGroupNames.size(), 2U);
        REQUIRE_EQUAL(engineData->captureGroupNames[0], StringEditor{"first"_el});
        REQUIRE_EQUAL(engineData->captureGroupNames[1], StringEditor{});
    }

    void testInvalidCharactersAndLineLength() {
        // Non-ASCII emoji triggers invalid character
        WITH_CONTEXT(requireCompilerError({"NONE🙂"}, "character"));

        // Line too long
        std::string longLine{";"};
        longLine += std::string(impl::limits::maximumAssemblerLineLength.toSizeT() + 1U, 'A');
        WITH_CONTEXT(requireCompilerError({longLine}, "Line too long"));
    }

    void testInvalidOperationName() {
        WITH_CONTEXT(requireCompilerError({"UNKNOWN"}), "Invalid operation name");
        auto tooLongOperation = std::string(101, 'A');
        el::text::StringList lines{String{StringEditor{tooLongOperation}}};
        WITH_CONTEXT(requireCompilerError(lines, "too long"));
    }

    void testLabelsBasicsAndErrors() {
        // Valid label attached to op
        WITH_CONTEXT(requireCompile({"start: NONE", "JUMP %start"}));
        WITH_CONTEXT(requireNone());
        WITH_CONTEXT(requireJump(0x0000));

        // Empty label
        WITH_CONTEXT(requireCompilerError({": \tNONE"}), "empty");

        // Label with invalid char
        WITH_CONTEXT(requireCompilerError({"bad label: NONE"}));

        // Label starting with digit
        WITH_CONTEXT(requireCompilerError({"1start: NONE"}), "cannot start with a digit");

        // Duplicate label
        WITH_CONTEXT(requireCompilerError({"dup: NONE", "dup: SUCCESS"}), "Duplicate label 'dup'");
    }

    void testRequireArgumentCount() {
        // Too many args for None
        WITH_CONTEXT(requireCompilerError({"None 1"}), "Operation 'None' requires 0 arguments");
        // Too few args for Jump
        WITH_CONTEXT(requireCompilerError({"Jump"}), "Operation 'Jump' requires 1 arguments");
    }

    void testInvalidCharacterAfterArgument() { WITH_CONTEXT(requireCompilerError({"JUMP 1 x"}), "character"); }

    void testInvalidCharacterInIdentifier() { WITH_CONTEXT(requireCompilerError({"CATEGORY &Lu!"}), "character"); }

    void testInvalidCharacterInInteger() { WITH_CONTEXT(requireCompilerError({"JUMP 123abc"}), "character"); }
};
