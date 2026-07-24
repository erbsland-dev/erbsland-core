// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/profiling/Statistics.hpp>
#include <erbsland/unittest/UnitTest.hpp>

namespace pf = erbsland::profiling;

TESTED_TARGETS(Statistics)
class StatisticsTest final : public el::UnitTest {
public:
    void testDistribution() {
        const auto result = pf::Statistics::calculate(el::List<double>{5.0, 1.0, 3.0, 2.0, 4.0});
        REQUIRE_EQUAL(result.minimum, 1.0);
        REQUIRE_EQUAL(result.median, 3.0);
        REQUIRE_EQUAL(result.mean, 3.0);
        REQUIRE_EQUAL(result.p95, 5.0);
        REQUIRE_EQUAL(result.maximum, 5.0);
    }

    void testEmpty() { REQUIRE_THROWS_AS(el::ApplicationError, pf::Statistics::calculate({})); }

    void testNormalizedRateAndFairness() {
        REQUIRE_EQUAL(pf::Statistics::normalizedRate(200U, el::TimeDelta::seconds(2)), 100.0);
        REQUIRE_EQUAL(pf::Statistics::normalizedRate(200U, {}), 0.0);
        REQUIRE_EQUAL(pf::Statistics::fairness(el::List<double>{100.0, 80.0, 90.0}), 0.8);
        REQUIRE_EQUAL(pf::Statistics::fairness({}), 1.0);
    }
};
