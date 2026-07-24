// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FakeScenarioWorkload.hpp"
#include "FakeTimeSource.hpp"

#include <erbsland/profiling/WorkloadRunner.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <memory>

namespace pf = erbsland::profiling;

using namespace el::text::literals;

TESTED_TARGETS(ScenarioWorkload WorkerWorkload WorkloadRunner)
class WorkloadRunnerTest final : public el::UnitTest {
public:
    void testOneWorker() { runSuccessful(1U); }

    void testFourWorkers() { runSuccessful(4U); }

    void testWorkerFailurePropagation() {
        auto executions = std::make_shared<std::atomic<std::uint64_t>>();
        auto validations = std::make_shared<std::atomic<std::uint64_t>>();
        auto definition = definitionFor(executions, validations, true);
        auto configuration = configurationFor(4U);
        REQUIRE_THROWS_AS(el::ApplicationError, pf::WorkloadRunner{definition, configuration}.run());
    }

    void testDeadline() {
        auto executions = std::make_shared<std::atomic<std::uint64_t>>();
        auto validations = std::make_shared<std::atomic<std::uint64_t>>();
        auto definition = definitionFor(executions, validations, false);
        auto configuration = configurationFor(1U);
        configuration.run.duration = el::TimeDelta::nanoseconds(1);
        auto timeSource = std::make_shared<FakeTimeSource>(el::TimeDelta::nanoseconds(1));
        auto runner = pf::WorkloadRunner{definition, configuration, std::move(timeSource)};
        REQUIRE_THROWS_AS(el::ApplicationError, runner.run());
    }

private:
    static auto definitionFor(
        const std::shared_ptr<std::atomic<std::uint64_t>> &executions,
        const std::shared_ptr<std::atomic<std::uint64_t>> &validations,
        const bool fail) -> pf::ProfilingDefinition {
        auto definition = pf::ProfilingDefinition{};
        definition.addFunctionality(
            {.id = "fake"_el,
                .factory = [executions, validations, fail](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<FakeScenarioWorkload>(executions, validations, fail);
                }});
        return definition;
    }

    static auto configurationFor(const std::uint32_t threads) -> pf::ProfilingConfiguration {
        auto configuration = pf::ProfilingConfiguration{};
        configuration.run.threadCount = threads;
        configuration.run.warmupSamples = 0U;
        configuration.run.samples = 1U;
        configuration.run.minimumSampleTime = {};
        configuration.run.duration = el::TimeDelta::seconds(1);
        configuration.scenarios.append({.id = "fake:scenario"_el, .group = "fake"_el, .functionality = "fake"_el});
        return configuration;
    }

    void runSuccessful(const std::uint32_t threads) {
        auto executions = std::make_shared<std::atomic<std::uint64_t>>();
        auto validations = std::make_shared<std::atomic<std::uint64_t>>();
        auto definition = definitionFor(executions, validations, false);
        auto configuration = configurationFor(threads);
        auto runner = pf::WorkloadRunner{definition, configuration};
        REQUIRE_EQUAL(runner.run(), el::ExitCode::success());
        REQUIRE_EQUAL(executions->load(), static_cast<std::uint64_t>(threads) * 2U);
        REQUIRE_EQUAL(validations->load(), 2U);
    }
};
