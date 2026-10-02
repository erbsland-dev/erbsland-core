// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Statistics.hpp"

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>

#include <cmath>
#include <limits>

using namespace el::text::literals;

/// Deterministic on-demand harness checks.
/// @tested{ConstantTimeStatisticsTest}
TESTED_TARGETS(Moments WelchTest Statistics Evidence)
class ConstantTimeStatisticsTest final : public el::UnitTest {
public:
    /// Online Moments And Welch.
    void testOnlineMomentsAndWelch() {
        auto moments = app::constant_time::Moments{};
        for (const auto value : {1.0, 2.0, 3.0}) {
            moments.add(value);
        }
        REQUIRE_EQUAL(moments.count(), 3U);
        REQUIRE_EQUAL(moments.mean(), 2.0);
        REQUIRE_EQUAL(moments.variance(), 1.0);
        auto test = app::constant_time::WelchTest{};
        for (const auto value : {1.0, 2.0, 3.0}) {
            test.add(value, false);
            test.add(value + 2, true);
        }
        REQUIRE(std::abs(test.statistic() + 2.0 / std::sqrt(2.0 / 3.0)) < 1e-12);
        REQUIRE_FALSE(test.isEligible());
        REQUIRE(test.isEligible(3));
    }
    /// Degenerate And Insufficient Populations.
    void testDegenerateAndInsufficientPopulations() {
        auto test = app::constant_time::WelchTest{};
        REQUIRE_EQUAL(test.statistic(), 0.0);
        for (auto index = 0; index < 10000; ++index) {
            test.add(5, false);
            test.add(5, true);
        }
        REQUIRE_EQUAL(test.statistic(), 0.0);
        REQUIRE_FALSE(test.isEligible());
        auto unequal = app::constant_time::WelchTest{};
        unequal.add(5, false);
        unequal.add(5, false);
        unequal.add(6, true);
        unequal.add(6, true);
        REQUIRE(std::isinf(unequal.statistic()));
    }
    /// Cropping And Mean Leakage.
    void testCroppingAndMeanLeakage() {
        auto statistics = app::constant_time::Statistics{};
        REQUIRE_THROWS_AS(el::ParameterError, statistics.initialize({}));
        auto pilot = el::List<double>{};
        for (auto index = 1; index <= 1000; ++index) {
            pilot.append(static_cast<double>(index));
        }
        statistics.initialize(pilot);
        REQUIRE(statistics.threshold(0) < statistics.threshold(99));
        REQUIRE_EQUAL(statistics.threshold(0), 67.0);
        REQUIRE_EQUAL(statistics.threshold(99), 1000.0);
        for (auto index = 0; index < 10000; ++index) {
            statistics.add(100.0 + index % 2, false);
            statistics.add(200.0 + index % 2, true);
        }
        REQUIRE(statistics.evidence().eligible);
        REQUIRE(statistics.evidence().samples[0] >= 10000);
        REQUIRE(statistics.evidence().samples[1] >= 10000);
        REQUIRE(std::abs(statistics.evidence().statistic) > 10);
        REQUIRE_FALSE(statistics.hasEnoughEvidence());
    }
    /// Second Order And Invalid Measurements.
    void testSecondOrderAndInvalidMeasurements() {
        auto statistics = app::constant_time::Statistics{};
        statistics.initialize(el::List<double>{1000.0, 1001.0, 1002.0});
        // Equal means, different variance; two signed amplitudes avoid mean drift.
        for (auto index = 0; index < 22000; ++index) {
            const auto sign = index % 2 == 0 ? -1.0 : 1.0;
            statistics.add(100.0 + sign, false);
            statistics.add(100.0 + 10.0 * sign, true);
        }
        REQUIRE(statistics.hasEnoughEvidence());
        REQUIRE(std::abs(statistics.evidence().statistic) > 10);
        REQUIRE_EQUAL(statistics.evidence().channel, "second-order"_el);
        const auto count = statistics.raw().population(false).count();
        statistics.add(0, false);
        statistics.add(-1, false);
        statistics.add(std::numeric_limits<double>::quiet_NaN(), false);
        REQUIRE_EQUAL(statistics.raw().population(false).count(), count);
    }
};
