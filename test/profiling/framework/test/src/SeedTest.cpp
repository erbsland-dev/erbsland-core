// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/profiling/Seed.hpp>
#include <erbsland/unittest/UnitTest.hpp>

namespace pf = erbsland::profiling;

using namespace el::text::literals;

TESTED_TARGETS(deriveSeed)
class SeedTest final : public el::UnitTest {
public:
    void testDeterminism() {
        const auto first = pf::deriveSeed(123U, "scenario"_el, 2U, 4U);
        REQUIRE_EQUAL(first, pf::deriveSeed(123U, "scenario"_el, 2U, 4U));
        REQUIRE_NOT_EQUAL(first, pf::deriveSeed(123U, "scenario"_el, 2U, 5U));
        REQUIRE_NOT_EQUAL(first, pf::deriveSeed(123U, "other"_el, 2U, 4U));
    }
};
