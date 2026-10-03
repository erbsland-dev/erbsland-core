// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include "impl/DataSourceTestState.hpp"
#include "impl/FirstDataSource.hpp"
#include "impl/PreCommitProbeTypes.hpp"
#include "impl/PreCommitTestTool.hpp"

#include <erbsland/ParameterFailure.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/WorkerPool.hpp>
#include <tool/anti_patterns/AntiPatternsTool.hpp>
#include <tool/frame/TaskProgress.hpp>
#include <tool/pre_commit/impl/TaskLogWriter.hpp>

using namespace test::impl;

TESTED_TARGETS(PreCommitTask PreCommitRunner ToolApplication TaskLogWriter TaskProgress)
class PreCommitRunnerTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
    /// Shared identifier for the probe that exercises failures and warnings.
    static constexpr auto cMiddleTask = "middle_task"_el;

public: // implement UnitTest
    void setUp() override {
        ProjectFixture::setUp();
        tool::test::impl::DataSourceTestState::shared() = {};
        PreCommitProbe::reset();
    }
    void tearDown() override {
        PreCommitProbe::reset();
        ProjectFixture::tearDown();
    }

public:
    void testRegistrationOrderDefaultsAndSingleInstance() {
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_EQUAL(PreCommitProbe::created, 3);
        REQUIRE(PreCommitProbe::progressActive);
        REQUIRE(PreCommitProbe::defaultsApplied);
        REQUIRE_EQUAL(PreCommitProbe::configured, PreCommitProbe::executed);
        REQUIRE_EQUAL(
            PreCommitProbe::events.join(","_el),
            "first_task:configure,first_task:initialize,first_task:execute,"
            "middle_task:configure,middle_task:initialize,middle_task:execute,"
            "last_task:configure,last_task:initialize,last_task:execute"_el);
        REQUIRE(_outputText.contains("3 passed, 0 failed"_el));
        REQUIRE_FALSE(_outputText.contains("information-detail"_el));
        REQUIRE_FALSE(_outputText.contains("trace-detail"_el));
        REQUIRE_FALSE(_outputText.contains("Errors"_el));
        REQUIRE_FALSE(_outputText.contains(el::String::fromCharacter(U'\u001b', el::CpCount{1})));
    }

    void testTaskOnlyRegistration() {
        REQUIRE_EQUAL(
            runTool<PreCommitTestTool>(
                {},
                [](TestToolApplication &app) -> void { app.registerPreCommitTask<TaskOnlyPreCommitProbe>(); },
                "test-tasks"_el),
            0);
        REQUIRE(_outputText.contains("task-only warning"_el));
        REQUIRE(_outputText.contains("1 passed, 0 failed"_el));
        REQUIRE_EQUAL(PreCommitProbe::created, 0);
    }

    void testSourcePreparationFailureBelongsToItsTask() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[first]\n"_el);
        PreCommitProbe::sourceUsers = el::StringList{cMiddleTask};
        tool::test::impl::DataSourceTestState::shared().failure = "first.initialize"_el;
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE(PreCommitProbe::events.contains("middle_task:initialize"_el));
        REQUIRE(PreCommitProbe::events.contains("last_task:execute"_el));
        REQUIRE(_outputText.contains("Deliberate failure in first.initialize"_el));
        REQUIRE(_outputText.contains("2 passed, 1 failed"_el));
    }

    void testSharedSourcesInitializeOnceAndUseTheCurrentTaskLog() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[first]\n"_el);
        PreCommitProbe::sourceUsers = el::StringList{"first_task"_el, "last_task"_el};
        PreCommitProbe::action = []() -> void {
            const auto name = PreCommitProbe::executed->identifier();
            if (PreCommitProbe::sourceUsers.contains(name)) {
                tool::frame::app().source<tool::test::impl::FirstDataSource>().logging()->warn(
                    name, " source warning"_el);
            }
        };
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_EQUAL(
            tool::test::impl::DataSourceTestState::shared().events.join(","_el), "first.configure,first.initialize"_el);
        REQUIRE(_outputText.contains("first_task source warning"_el));
        REQUIRE(_outputText.contains("last_task source warning"_el));
        REQUIRE(_outputText.find("[RUN] last_task"_el) < _outputText.find("last_task source warning"_el));
    }

    void testEveryOperationalFailureContinues() {
        for (const auto &phase : el::StringList{"configure"_el, "initialize"_el, "execute"_el}) {
            PreCommitProbe::reset();
            PreCommitProbe::failures.set(cMiddleTask, phase);
            REQUIRE_EQUAL(run(), 1);
            REQUIRE(PreCommitProbe::events.contains("last_task:execute"_el));
            REQUIRE(_outputText.contains(el::join("middle_task failed during "_el, phase)));
            REQUIRE(_outputText.contains("2 passed, 1 failed"_el));
        }
    }

    void testConfigurationValidationFailureIsLocal() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[middle_task]\n"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE_FALSE(PreCommitProbe::events.contains("middle_task:configure"_el));
        REQUIRE(PreCommitProbe::events.contains("last_task:execute"_el));
        REQUIRE(_outputText.contains("must not have a configuration section"_el));
    }

    void testMultipleFailuresAndWarnings() {
        PreCommitProbe::failures.set("first_task"_el, "configure"_el);
        PreCommitProbe::messages.set(cMiddleTask, "warning"_el);
        PreCommitProbe::messages.set("last_task"_el, "error"_el);
        REQUIRE_EQUAL(run(), 1);
        REQUIRE(_outputText.contains("1 passed, 2 failed"_el));
        REQUIRE(_outputText.contains("middle_task warning-detail"_el));
        REQUIRE(_outputText.contains("last_task error-detail"_el));
    }

    void testWarningOnlyRunSucceeds() {
        PreCommitProbe::messages.set(cMiddleTask, "warning"_el);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE(_outputText.contains("middle_task warning-detail"_el));
        REQUIRE(_outputText.contains("(warnings)"_el));
    }

    void testVerboseAndFileLogging() {
        REQUIRE_EQUAL(run(el::StringList{"--verbose"_el, "--log-file"_el, (_root / "task.log"_el).toString()}), 0);
        REQUIRE(_outputText.contains("first_task information-detail"_el));
        REQUIRE(_outputText.contains("last_task trace-detail"_el));
        const auto log = (_root / "task.log"_el).content().readTextOrThrow();
        REQUIRE(log.contains("pre_commit/first_task"_el));
        REQUIRE(log.contains("last_task trace-detail"_el));
    }

    void testQueuedLogsAreDrainedAndAttributed() {
        PreCommitProbe::burstLogging = true;
        REQUIRE_EQUAL(run(el::StringList{"--verbose"_el}), 0);
        for (const auto &name : el::StringList{"first_task"_el, cMiddleTask, "last_task"_el}) {
            REQUIRE(_outputText.contains(el::join(name, " drained-entry 99"_el)));
        }
        REQUIRE(_outputText.find("first_task drained-entry 99"_el) < _outputText.find("[RUN] middle_task"_el));
        REQUIRE(_outputText.find("middle_task drained-entry 99"_el) < _outputText.find("[RUN] last_task"_el));
    }

    void testAntiPatternReportsRemainVisibleWithoutVerbose() {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[anti_patterns]\nsource_directories: \"src\"\nfile_suffixes: \".cpp\"\n"_el);
        write("src/Example.cpp"_el, "namespace {\n}\n"_el);
        REQUIRE_EQUAL(
            runTool<PreCommitTestTool>(
                {},
                [](TestToolApplication &app) -> void {
                    app.registerPreCommitTask<tool::anti_patterns::AntiPatternsTool>();
                },
                "test-tasks"_el),
            1);
        REQUIRE(_outputText.contains("HIGH anonymous_namespace src/Example.cpp:1"_el));
        REQUIRE(_outputText.contains("doc/guidelines/anti_patterns/anonymous_namespace.rst"_el));
        REQUIRE(_outputText.contains("1 active"_el));
        REQUIRE(_outputText.contains("0 passed, 1 failed"_el));
    }

    void testStandaloneCommandUsesSameBodyAndInactiveProgress() {
        REQUIRE_EQUAL(runTool<FirstPreCommitProbe>({}, {}, "first-task"_el), 0);
        REQUIRE_EQUAL(PreCommitProbe::configured, PreCommitProbe::executed);
        REQUIRE(PreCommitProbe::defaultsApplied);
        REQUIRE(_outputText.contains("information-detail"_el));
        REQUIRE_FALSE(_outputText.contains("[RUN]"_el));
        REQUIRE_FALSE(PreCommitProbe::progressActive);
    }

    void testProgressCountersAndInactiveHandle() {
        const auto inactive = tool::frame::TaskProgress{};
        inactive.beginPhase("ignored"_el, el::ItemCount{0});
        inactive.advance();
        REQUIRE(inactive.snapshot().phase.isEmpty());
        const auto progress = tool::frame::TaskProgress::create();
        progress.beginPhase("discover"_el);
        REQUIRE_FALSE(progress.snapshot().total.has_value());
        progress.beginPhase("files"_el, el::ItemCount{100});
        auto jobs = el::List<int>{};
        for (auto index = 0; index < 100; ++index) {
            jobs.append(index);
        }
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(4));
        pool->forEach(jobs, [progress](int) -> void { progress.advance(); }).get();
        REQUIRE_EQUAL(progress.snapshot().completed, el::ItemCount{100});
        REQUIRE_THROWS_AS(el::ParameterFailure, progress.advance());
        progress.beginPhase("next"_el, el::ItemCount{0});
        REQUIRE(progress.snapshot().completed.isZero());
    }

private:
    auto run(const el::StringList &arguments = {}) -> int {
        return runTool<PreCommitTestTool>(
            {},
            [](TestToolApplication &app) -> void {
                app.registerSource<tool::test::impl::FirstDataSource>();
                app.registerPreCommitTask<FirstPreCommitProbe>();
                app.registerPreCommitTask<MiddlePreCommitProbe>();
                app.registerPreCommitTask<LastPreCommitProbe>();
            },
            "test-tasks"_el,
            arguments);
    }
};
