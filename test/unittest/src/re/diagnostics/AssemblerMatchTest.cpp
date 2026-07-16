// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

TESTED_TARGETS(Assembler)
TAGS(Diagnostics)
class AssemblerMatchTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testMatchOperations() {
        WITH_CONTEXT(requireCompile({
            "MATCH",
            "NOT MATCH",
            "SUCCESS",
            "FAILURE",
        }));
        WITH_CONTEXT(requireMatch());
        WITH_CONTEXT(requireNotMatch());
        WITH_CONTEXT(requireSuccess());
        WITH_CONTEXT(requireFailure());
    }

    void testMatchArgumentErrors() {
        WITH_CONTEXT(requireCompilerError({"MATCH 1"}));
        WITH_CONTEXT(requireCompilerError({"NOT MATCH 1"}));
        WITH_CONTEXT(requireCompilerError({"SUCCESS 1"}));
        WITH_CONTEXT(requireCompilerError({"FAILURE 1"}));
    }
};
