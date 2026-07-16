// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

TESTED_TARGETS(Assembler)
TAGS(Diagnostics)
class AssemblerAnchorTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testAnchorOperation() {
        WITH_CONTEXT(requireCompile({
            "ANCHOR &LineStart",
            "ANCHOR &linEend", // case-insensitive
        }));
        WITH_CONTEXT(requireAnchor(TextAnchor::LineStart));
        WITH_CONTEXT(requireAnchor(TextAnchor::LineEnd));

        WITH_CONTEXT(requireCompilerError({"ANCHOR &NotAnAnchor"}), "Invalid anchor 'NotAnAnchor'");
        WITH_CONTEXT(requireCompilerError({"ANCHOR 123"}));
        WITH_CONTEXT(requireCompilerError({"ANCHOR false"}));
    }
};
