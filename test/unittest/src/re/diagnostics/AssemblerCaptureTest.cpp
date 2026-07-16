// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

TESTED_TARGETS(Assembler)
TAGS(Diagnostics)
class AssemblerCaptureTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testCaptureOperations() {
        WITH_CONTEXT(requireCompile({
            ".groups 2",
            "START CAPTURE 1",
            "STOP CAPTURE 1",
        }));
        WITH_CONTEXT(requireStartCapture(1));
        WITH_CONTEXT(requireStopCapture(1));

        // Errors
        WITH_CONTEXT(requireCompilerError({"START CAPTURE true"}));
        WITH_CONTEXT(requireCompilerError({"STOP CAPTURE &text"}));
        WITH_CONTEXT(requireCompilerError({"START CAPTURE 1x"}));
    }

    void testCaptureIndexOutOfRange() {
        WITH_CONTEXT(
            requireCompilerError({
                ".groups 2",
                "START CAPTURE 2",
            }),
            "out of range");
        WITH_CONTEXT(
            requireCompilerError({
                ".groups 2",
                "STOP CAPTURE 2",
            }),
            "out of range");
    }
};
