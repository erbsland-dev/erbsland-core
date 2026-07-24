// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

#include <erbsland/re/StdFormat.hpp>

TESTED_TARGETS(Assembler)
TAGS(Diagnostics)
class AssemblerFlowTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testFlowOperationsAndJumpPatching() {
        // Split with forward and backward labels; Jump forward; Success/Failure
        WITH_CONTEXT(requireCompile(
            {"entry: NONE",
                "SPLIT %entry, %end", // backward known, forward patch
                "FAILURE",
                "JUMP %end",          // forward patch
                "end: SUCCESS"}));
        WITH_CONTEXT(requireNone());
        WITH_CONTEXT(requireSplit(0x0000, 0x0005));
        WITH_CONTEXT(requireFailure());
        WITH_CONTEXT(requireJump(0x0005));
        WITH_CONTEXT(requireSuccess());
    }

    void testAtomicOperations() {
        WITH_CONTEXT(requireCompile({
            "START ATOMIC 0",
            "STOP ATOMIC 0",
        }));
        WITH_CONTEXT(requireStartAtomic(0));
        WITH_CONTEXT(requireStopAtomic(0));

        WITH_CONTEXT(requireCompilerError({"START ATOMIC"}));
        WITH_CONTEXT(requireCompilerError({"STOP ATOMIC"}));
    }

    void testProgramCounterParsingErrors() {
        WITH_CONTEXT(requireCompilerError({"JUMP ,"}));
        WITH_CONTEXT(requireCompilerError({"JUMP 12x"}));
        WITH_CONTEXT(requireCompilerError({"JUMP %abc!"}));
        WITH_CONTEXT(requireCompilerError({"JUMP false"}));
        auto tooLongLabel = std::string(101, 'a');
        WITH_CONTEXT(requireCompilerError({std::format("JUMP %{}", tooLongLabel)}), "too long");
    }

    void testJumpPatchingErrorForMissingLabel() {
        WITH_CONTEXT(requireCompilerError({"JUMP %abc", "NONE"}), "not found");
    }

    void testJumpToWrongSection() {
        WITH_CONTEXT(
            requireCompilerError({".class", "c1: CHAR 'a'", ".program", "JUMP %c1"}), "points to a 'class' section");
    }
};
