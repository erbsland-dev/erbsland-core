// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssemblerBase.hpp"

#include <erbsland/re/StdFormat.hpp>

TESTED_TARGETS(Assembler)
TAGS(Diagnostics)
class AssemblerCounterTest final : public UNITTEST_SUBCLASS(AssemblerBase) {
public:
    void testCounterOperations() {
        WITH_CONTEXT(requireCompile({
            "COUNTER 0, 1",
            "ADD COUNTER 1, 2",
            "MAXIMUM 1, 3",
            "SKIP MAXIMUM 0, 3",
            "MINIMUM 0, 1",
        }));
        WITH_CONTEXT(requireCounter(0, 1));
        WITH_CONTEXT(requireAddCounter(1, 2));
        WITH_CONTEXT(requireMaximum(1, 3));
        WITH_CONTEXT(requireSkipMaximum(0, 3));
        WITH_CONTEXT(requireMinimum(0, 1));
    }

    void testCounterOperationErrors() {
        WITH_CONTEXT(requireCompilerError({"COUNTER"}));
        WITH_CONTEXT(requireCompilerError({"ADD COUNTER"}));
        WITH_CONTEXT(requireCompilerError({"MAXIMUM"}));
        WITH_CONTEXT(requireCompilerError({"SKIP MAXIMUM"}));
        WITH_CONTEXT(requireCompilerError({"MINIMUM"}));

        WITH_CONTEXT(requireCompilerError({"COUNTER false, 1"}));
        WITH_CONTEXT(requireCompilerError({"COUNTER 0, false"}));
        WITH_CONTEXT(requireCompilerError({"COUNTER 0x, 1"}), "integer format");
        WITH_CONTEXT(requireCompilerError({"COUNTER 0, 1x"}), "integer format");
        WITH_CONTEXT(requireCompilerError({"COUNTER &x, 1"}));
        WITH_CONTEXT(requireCompilerError({"COUNTER 0, &x"}));
    }

    void testCounterLimits() {
        // Index is zero-based, and the maximum number of counters is fixed.
        WITH_CONTEXT(requireCompile({
            std::format("COUNTER {}, 0", impl::limits::maximumCounterCount - 1),
        }));
        WITH_CONTEXT(requireCounter(static_cast<CounterIndex>(impl::limits::maximumCounterCount - 1), 0));

        WITH_CONTEXT(
            requireCompilerError({
                std::format("COUNTER {}, 0", impl::limits::maximumCounterCount),
            }),
            "out of range");

        // Value is stored in a single code unit (lower word in the VM).
        WITH_CONTEXT(requireCompilerError({"COUNTER 0, 65536"}), "out of range");
    }
};
