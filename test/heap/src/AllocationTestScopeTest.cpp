// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AllocationTestScope.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(AllocationTestScope)
class AllocationTestScopeTest final : public el::UnitTest {
public:
    void testCountsOnlyInsideScope() {
        auto emptyScope = erbsland::test::AllocationTestScope{};
        const auto emptyAllocations = emptyScope.finish();
        REQUIRE_EQUAL(emptyAllocations, 0U);

        auto scope = erbsland::test::AllocationTestScope{};
        auto *memory = ::operator new(64U);
        const auto allocations = scope.finish();
        ::operator delete(memory);
        REQUIRE_EQUAL(allocations, 1U);

        auto *outsideMemory = ::operator new(64U);
        ::operator delete(outsideMemory);
        auto finalScope = erbsland::test::AllocationTestScope{};
        const auto finalAllocations = finalScope.finish();
        REQUIRE_EQUAL(finalAllocations, 0U);
    }
};
