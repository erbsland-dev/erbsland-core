// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompilerBase.hpp"

#include <erbsland/re/StdFormatForRegEx.hpp>

TESTED_TARGETS(Compiler)
TAGS(Compilation)
class CompilerLimitTest final : public UNITTEST_SUBCLASS(CompilerBase) {
public:
    void testMaximumProgramLengthExceeded() {
        // Create a pattern that generates a program longer than maximumProgramLength (65534).
        // Each `^` anchor generates one ANCHOR operation.
        // We need > 65534 operations.

        const auto pattern = String::fromCharacter(el::text::Char{U'^'}, el::unit::CpLength{66'000U});

        try {
            compileAndDisassemble(pattern);
            REQUIRE(false); // Should have thrown
        } catch (const el::re::RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Limit);
            REQUIRE_EQUAL(e.title(), "Failed to compile regular expression"_el);
            REQUIRE_EQUAL(e.description(), "The generated program exceeds the maximum program length."_el);
        }
    }
};
