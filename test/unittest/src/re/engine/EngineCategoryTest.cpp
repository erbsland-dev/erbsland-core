// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineCategoryTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testNormalCategory() {
        WITH_CONTEXT(assembleProgram({
            "          CATEGORY &Letter",
            "          CATEGORY &DecimalNumber",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("z9"_el, 2));
        WITH_CONTEXT(requireNoMatch("az"_el));
        WITH_CONTEXT(requireNoMatch("1z"_el));
    }

    void testNegatedCategory() {
        WITH_CONTEXT(assembleProgram({
            "          NOT CATEGORY &Letter",
            "          NOT CATEGORY &DecimalNumber",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("😄👍"_el, 8));
        WITH_CONTEXT(requireNoMatch("a😄"_el));
        WITH_CONTEXT(requireNoMatch("😄5"_el));
    }
};
