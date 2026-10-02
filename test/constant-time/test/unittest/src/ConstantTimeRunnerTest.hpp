// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstantTimeProbe.hpp"
#include "Registry.hpp"
#include "Runner.hpp"
#include "RunResults.hpp"
#include "SelfTest.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimePoint.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

/// Deterministic on-demand harness checks.
/// @tested{ConstantTimeRunnerTest}
TESTED_TARGETS(Registration Registry Runner RunOptions Result TestMetadata RunResults SelfTest)
class ConstantTimeRunnerTest final : public el::UnitTest {
public:
    /// Selection And Deduplication.
    void testSelectionAndDeduplication() {
        REQUIRE_THROWS_AS(
            el::LogicError, app::constant_time::Registration{std::shared_ptr<const app::constant_time::TestCase>{}});
        const auto descriptionOnly = app::constant_time::Registration{"description-only"_el};
        REQUIRE_THROWS_AS(el::LogicError, descriptionOnly.create(false));
        auto registry = app::constant_time::Registry{};
        registry.add({"first"_el, {}});
        registry.add({"second"_el, {}});
        REQUIRE_EQUAL(registry.select(el::StringList{"second"_el, "second"_el}, false).count(), el::ItemCount{1});
        REQUIRE_EQUAL(registry.select({}, true).count(), el::ItemCount{2});
        REQUIRE_THROWS_AS(el::ParameterError, registry.select(el::StringList{"unknown"_el}, false));
        REQUIRE_THROWS_AS(el::ParameterError, registry.select(el::StringList{"first"_el}, true));
        REQUIRE_THROWS_AS(el::ParameterError, registry.add({"first"_el, {}}));
    }
    /// Sample Limit And Cleanup.
    void testSampleLimitAndCleanup() {
        auto probe = ConstantTimeProbe{};
        auto runner = app::constant_time::Runner{[&probe]() -> el::TimePoint { return probe.now; }};
        const auto result = runner.run(probe, {.duration = el::Seconds{5}, .maximumSamples = 64, .seed = 42});
        REQUIRE_EQUAL(result.samples(false) + result.samples(true), 64U);
        REQUIRE(result.outcome() == app::constant_time::Outcome::Insufficient);
        REQUIRE_EQUAL(probe.prepared, 0U);
        REQUIRE_EQUAL(result.repetitions(), 1U);
    }
    /// Keep measured fixtures and labels stable when pilot sizes vary with preparation timing.
    void testSeedDoesNotDependOnPilotBatchSize() {
        auto fast = ConstantTimeProbe{};
        fast.capture = true;
        auto slow = ConstantTimeProbe{};
        slow.capture = true;
        slow.prepareDelay = el::Milliseconds{100};
        auto fastRunner = app::constant_time::Runner{[&fast]() -> el::TimePoint { return fast.now; }};
        auto slowRunner = app::constant_time::Runner{[&slow]() -> el::TimePoint { return slow.now; }};
        const auto options =
            app::constant_time::RunOptions{.duration = el::Seconds{60}, .maximumSamples = 64, .seed = 42};
        const auto fastResult = fastRunner.run(fast, options);
        const auto slowResult = slowRunner.run(slow, options);
        REQUIRE_EQUAL(fastResult.samples(false) + fastResult.samples(true), 64U);
        REQUIRE_EQUAL(slowResult.samples(false) + slowResult.samples(true), 64U);
        REQUIRE(fast.calls != slow.calls);
        for (auto index = std::size_t{}; index < 64; ++index) {
            const auto fastPosition = el::ItemIndex{fast.trace.count().toSizeT() - 64 + index};
            const auto slowPosition = el::ItemIndex{slow.trace.count().toSizeT() - 64 + index};
            REQUIRE_EQUAL(fast.trace.getRefOrThrow(fastPosition), slow.trace.getRefOrThrow(slowPosition));
        }
    }
    /// Budget During Preparation And Errors.
    void testBudgetDuringPreparationAndErrors() {
        auto probe = ConstantTimeProbe{};
        auto runner = app::constant_time::Runner{[&probe]() -> el::TimePoint { return probe.now; }};
        const auto result = runner.run(probe, {.duration = el::Microseconds{1}, .seed = 42});
        REQUIRE(result.outcome() == app::constant_time::Outcome::Insufficient);
        REQUIRE_EQUAL(probe.calls, 0U);
        probe.fail = true;
        REQUIRE(runner.run(probe, {}).outcome() == app::constant_time::Outcome::Error);
        REQUIRE(runner.run(probe, {.duration = el::TimeDelta{}}).outcome() == app::constant_time::Outcome::Error);
    }
    /// Factory Budget Memory And Timer Failures.
    void testFactoryBudgetMemoryAndTimerFailures() {
        auto probe = ConstantTimeProbe{};
        auto runner = app::constant_time::Runner{[&probe]() -> el::TimePoint { return probe.now; }};
        const auto start = probe.now;
        probe.now += el::Seconds{2};
        REQUIRE(
            runner.run(probe, {.duration = el::Seconds{1}, .start = start}).outcome() ==
            app::constant_time::Outcome::Insufficient);
        REQUIRE_EQUAL(probe.calls, 0U);
        probe.bytes = el::ByteLength{64U * 1024U * 1024U + 1};
        REQUIRE(runner.run(probe, {}).outcome() == app::constant_time::Outcome::Error);
        probe.bytes = el::ByteLength{1024};
        auto stationary = app::constant_time::Runner{[]() -> el::TimePoint { return {}; }};
        const auto result = stationary.run(probe, {});
        REQUIRE(result.outcome() == app::constant_time::Outcome::Insufficient);
        REQUIRE(result.detail().contains("timer"_el));
    }
    /// Outcome Precedence And Reporting.
    void testOutcomePrecedenceAndReporting() {
        using namespace app::constant_time;
        auto results = RunResults{};
        results.append(Result{{}, Outcome::NoLeakage});
        REQUIRE_EQUAL(results.exitCode(), 0);
        results.append(Result{{}, Outcome::Insufficient});
        REQUIRE_EQUAL(results.exitCode(), 3);
        results.append(Result{{}, Outcome::Leakage});
        REQUIRE_EQUAL(results.exitCode(), 1);
        results.append(Result{{}, Outcome::Error});
        REQUIRE_EQUAL(results.exitCode(), 2);
        results.append(Result{{}, Outcome::Interrupted});
        REQUIRE_EQUAL(results.exitCode(), 130);
        const auto result = Result{TestMetadata{.id = "probe"_el}, Outcome::Insufficient, 42};
        const auto report = result.toString();
        REQUIRE(report.contains("test=probe"_el));
        REQUIRE(report.contains("insufficient-evidence"_el));
        REQUIRE(report.contains("seed=42"_el));
        REQUIRE(report.contains("analysis-samples-0=0"_el));
        REQUIRE_FALSE(report.contains("\033"_el));
    }
    /// Interruption And Self Checks.
    void testInterruptionAndSelfChecks() {
        auto probe = ConstantTimeProbe{};
        auto interrupted = std::atomic<bool>{true};
        auto runner = app::constant_time::Runner{[&probe]() -> el::TimePoint { return probe.now; }, &interrupted};
        REQUIRE(runner.run(probe, {}).outcome() == app::constant_time::Outcome::Interrupted);
        app::constant_time::SelfTest{}.checkDeterministic();
    }
};
