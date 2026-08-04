// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/profiling/SampleMeasurement.hpp>
#include <erbsland/profiling/Statistics.hpp>
#include <erbsland/profiling/WorkerMeasurement.hpp>
#include <erbsland/unittest/UnitTest.hpp>

namespace pf = erbsland::profiling;

TESTED_TARGETS(Statistics)
class StatisticsTest final : public el::UnitTest {
public:
    void testDistribution() {
        const auto result = pf::Statistics{el::List<double>{5.0, 1.0, 3.0, 2.0, 4.0}};
        REQUIRE_EQUAL(result.minimum(), 1.0);
        REQUIRE_EQUAL(result.median(), 3.0);
        REQUIRE_EQUAL(result.mean(), 3.0);
        REQUIRE_EQUAL(result.p95(), 5.0);
        REQUIRE_EQUAL(result.maximum(), 5.0);
    }

    void testEmpty() { REQUIRE_THROWS_AS(el::ApplicationError, pf::Statistics(el::List<double>{})); }

    void testNormalizedRateAndFairness() {
        REQUIRE_EQUAL(pf::Statistics::normalizedRate(200U, el::TimeDelta::seconds(2)), 100.0);
        REQUIRE_EQUAL(pf::Statistics::normalizedRate(200U, {}), 0.0);
        REQUIRE_EQUAL(pf::Statistics::fairness(el::List<double>{100.0, 80.0, 90.0}), 0.8);
        REQUIRE_EQUAL(pf::Statistics::fairness({}), 1.0);
    }

    void testBenchmarkAggregation() {
        auto first = pf::SampleMeasurement{
            .workers = el::List<pf::WorkerMeasurement>{pf::WorkerMeasurement{
                .operations = 100U, .elapsed = el::TimeDelta::seconds(1)}},
            .metrics = el::List<std::uint64_t>{10U, 20U},
            .operations = 100U,
            .wallTime = el::TimeDelta::seconds(1)};
        auto second = pf::SampleMeasurement{
            .workers = el::List<pf::WorkerMeasurement>{pf::WorkerMeasurement{
                .operations = 100U, .elapsed = el::TimeDelta::seconds(2)}},
            .metrics = el::List<std::uint64_t>{30U, 40U},
            .operations = 100U,
            .wallTime = el::TimeDelta::seconds(2)};

        const auto result = pf::Statistics{el::List<pf::SampleMeasurement>{std::move(first), std::move(second)}};
        REQUIRE_EQUAL(result.minimum(), 10'000'000.0);
        REQUIRE_EQUAL(result.median(), 20'000'000.0);
        REQUIRE_EQUAL(result.mean(), 15'000'000.0);
        REQUIRE_EQUAL(result.workerFairness(), 0.5);
        REQUIRE_EQUAL(result.totalElapsed(), el::TimeDelta::seconds(3));
        REQUIRE_EQUAL(result.metricTotal(0U), 40U);
        REQUIRE_EQUAL(result.metricTotal(1U), 60U);
    }
};
