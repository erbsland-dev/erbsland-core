// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

TESTED_TARGETS(Assembler)
TAGS(Diagnostics) class AssemblerCategoryTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testCategoryOperation() {
        WITH_CONTEXT(requireCompile({
            "CATEGORY &Lu",
            "NOT CATEGORY &Uppercase_Letter",
            "ASSERT CATEGORY &Lu",
            "NOT ASSERT CATEGORY &Uppercase_Letter",
        }));
        WITH_CONTEXT(requireCategory(Category::UppercaseLetter));
        WITH_CONTEXT(requireNotCategory(Category::UppercaseLetter));
        WITH_CONTEXT(requireAssertCategory(Category::UppercaseLetter));
        WITH_CONTEXT(requireNotAssertCategory(Category::UppercaseLetter));

        WITH_CONTEXT(requireCompilerError({"CATEGORY &NotACategory"}), "category");
    }

    void testAnyOperation() {
        WITH_CONTEXT(requireCompile({
            "ANY",
        }));
        WITH_CONTEXT(requireAny());

        WITH_CONTEXT(requireCompilerError({"ANY 1"}));
    }
};
