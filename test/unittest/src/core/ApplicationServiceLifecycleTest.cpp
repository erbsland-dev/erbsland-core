// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/system/impl/service/ServiceLifecycle.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/unit/ExitCode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace application_service_lifecycle_test {

struct LifecycleState {
    std::size_t runCount{};
    std::vector<std::int64_t> waitHints;
    std::size_t completeCount{};
    std::size_t stoppingCount{};
    std::vector<std::string> sequence;
};

class TestServiceLifecycle final : public el::system::impl::ServiceLifecycle {
public:
    TestServiceLifecycle(StopFn stopFn, std::shared_ptr<LifecycleState> state) :
        _stopFn{std::move(stopFn)}, _state{std::move(state)} {}

    [[nodiscard]] auto run(RunFn runFn) -> int override {
        ++_state->runCount;
        return runFn();
    }

    void reportStartupPending(const el::time::TimeDelta expectedRemainingTime) override {
        _state->waitHints.emplace_back(expectedRemainingTime.toMilliseconds().toRawValue());
        _state->sequence.emplace_back("pending");
    }

    void reportStartupComplete() override {
        ++_state->completeCount;
        _state->sequence.emplace_back("complete");
    }

    void reportStopping() noexcept override {
        ++_state->stoppingCount;
        _state->sequence.emplace_back("stopping");
    }

private:
    StopFn _stopFn;
    std::shared_ptr<LifecycleState> _state;
};

class TestApplication final : public el::core::Application {
public:
    [[nodiscard]] auto lifecycleState() const noexcept -> const std::shared_ptr<LifecycleState> & { return _state; }

protected:
    void initializeServiceLifecycle(el::system::impl::ServiceLifecyclePtr &serviceLifecycle) noexcept override {
        serviceLifecycle = std::make_unique<TestServiceLifecycle>([this]() -> void { quit(); }, _state);
    }

private:
    std::shared_ptr<LifecycleState> _state{std::make_shared<LifecycleState>()};
};

}

TESTED_TARGETS(Application)
class ApplicationServiceLifecycleTest final : public el::UnitTest {
public:
    void testEnabledForegroundLifecycleRunsApplication() {
        auto scope = ApplicationTestScope<application_service_lifecycle_test::TestApplication>{};
        auto &application = scope.app();
        const auto state = application.lifecycleState();
        auto readyBeforeMain = false;
        application.enableServiceLifecycle();
        application.enableServiceLifecycle();
        application.setMainFn([state, &readyBeforeMain]() -> el::unit::ExitCode {
            readyBeforeMain = state->completeCount == 1U;
            return el::unit::ExitCode{17};
        });

        REQUIRE_EQUAL(application.run(), 17);
        REQUIRE(readyBeforeMain);
        REQUIRE_EQUAL(state->runCount, 1U);
        REQUIRE_EQUAL(state->completeCount, 1U);
    }

    void testExplicitStartupProgressAndCompletion() {
        auto scope = ApplicationTestScope<application_service_lifecycle_test::TestApplication>{};
        auto &application = scope.app();
        const auto state = application.lifecycleState();
        application.enableServiceLifecycle();
        application.reportStartupPending(el::time::TimeDelta::seconds(20));
        application.reportStartupPending(el::time::TimeDelta::seconds(10));
        application.reportStartupComplete();
        application.reportStartupComplete();

        REQUIRE_EQUAL(state->waitHints, (std::vector<std::int64_t>{20'000, 10'000}));
        REQUIRE_EQUAL(state->completeCount, 1U);
        REQUIRE_THROWS_AS(el::err::LogicError, application.reportStartupPending(el::time::TimeDelta::seconds(1)));
    }

    void testStartupReportsRequireEnabledLifecycleAndPositiveTime() {
        auto scope = ApplicationTestScope<application_service_lifecycle_test::TestApplication>{};
        auto &application = scope.app();
        REQUIRE_THROWS_AS(el::err::LogicError, application.reportStartupPending(el::time::TimeDelta::seconds(1)));
        REQUIRE_THROWS_AS(el::err::LogicError, application.reportStartupComplete());

        application.enableServiceLifecycle();
        REQUIRE_THROWS_AS(el::err::ParameterError, application.reportStartupPending(el::time::TimeDelta::zero()));
        REQUIRE_THROWS_AS(el::err::ParameterError, application.reportStartupPending(el::time::TimeDelta::seconds(-1)));
    }

    void testPartsDelayAutomaticReadinessUntilRunning() {
        auto scope = ApplicationTestScope<application_service_lifecycle_test::TestApplication>{};
        auto &application = scope.app();
        const auto state = application.lifecycleState();
        application.enableServiceLifecycle();
        const auto manager = application.partManager();
        auto stopSubscription = manager->events().addStateChanged(
            [&application](const el::core::ApplicationPartManagerState managerState) -> void {
                if (managerState == el::core::ApplicationPartManagerState::Running) {
                    application.quit();
                }
            });

        REQUIRE_EQUAL(application.run(), 0);
        REQUIRE_EQUAL(state->sequence[0], "pending");
        REQUIRE_EQUAL(state->sequence[1], "complete");
        REQUIRE_EQUAL(state->stoppingCount, 1U);
    }

    void testShutdownDuringInitializationPreservesFirstExitCode() {
        auto scope = ApplicationTestScope<application_service_lifecycle_test::TestApplication>{};
        auto &application = scope.app();
        const auto state = application.lifecycleState();
        application.enableServiceLifecycle();
        application.setInitializeFn([&application]() -> void {
            application.quit(el::unit::ExitCode{7});
            application.quit(el::unit::ExitCode{9});
        });

        REQUIRE_EQUAL(application.run(), 7);
        REQUIRE_EQUAL(state->completeCount, 0U);
        REQUIRE_EQUAL(state->stoppingCount, 1U);
    }

    void testLifecycleCannotBeEnabledAfterRun() {
        auto scope = ApplicationTestScope<application_service_lifecycle_test::TestApplication>{};
        auto &application = scope.app();
        application.setMainFn([]() -> el::unit::ExitCode { return el::unit::ExitCode::success(); });
        REQUIRE_EQUAL(application.run(), 0);
        REQUIRE_THROWS_AS(el::err::LogicError, application.enableServiceLifecycle());
    }

    void testLifecycleStateIsSharedBetweenApplicationInstances() {
        auto scope = ApplicationTestScope<application_service_lifecycle_test::TestApplication>{};
        auto &application = scope.app();
        const auto state = application.lifecycleState();
        application.enableServiceLifecycle();
        {
            auto secondaryApplication = application_service_lifecycle_test::TestApplication{};
            secondaryApplication.reportStartupPending(el::time::TimeDelta::seconds(8));
        }
        application.reportStartupComplete();

        REQUIRE_EQUAL(state->waitHints, (std::vector<std::int64_t>{8'000}));
        REQUIRE_EQUAL(state->completeCount, 1U);
    }
};
