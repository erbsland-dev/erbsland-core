// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CurrentLoop.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/Utilities.hpp>
#include <erbsland/WorkerPoolFullError.hpp>

#include <latch>
#include <vector>

TESTED_TARGETS(Utilities UtilitiesBackend)
class UtilitiesTest final : public el::UnitTest {
public:
    void testSelectedLoopBindingAndDeferredDelivery() {
        const auto first = el::EventLoop::create();
        const auto second = el::EventLoop::create();
        auto pool = el::WorkerPoolPtr{};
        auto calls = std::vector<int>{};
        second->invoke([&]() -> void {
            auto &utilities = second->get<el::Utilities>();
            REQUIRE_EQUAL(&utilities, &second->get<el::Utilities>());
            REQUIRE_NOT_EQUAL(&utilities, &first->get<el::Utilities>());
            REQUIRE_EQUAL(utilities.backendId(), el::EventRegistry::utilitiesBackend());
            pool = utilities.createWorkerPool(el::WorkerPoolOptions{}.setWorkerCount(2));
            REQUIRE_EQUAL(pool->ownerLoop(), second);
            REQUIRE_EQUAL(pool->workerCount(), 2U);
            REQUIRE_EQUAL(pool->events().target(), second);
            pool->events().onFinal([&]() -> void { calls.push_back(2); });
            pool->submit(
                []() -> int { return 42; },
                [&](int result) -> void {
                    REQUIRE_EQUAL(el::currentLoop(), second);
                    REQUIRE_EQUAL(result, 42);
                    calls.push_back(1);
                });
            pool->quit();
            pool->join();
            REQUIRE(calls.empty());
        });
        static_cast<void>(second->runUntilIdle());
        REQUIRE_EQUAL(calls, (std::vector<int>{1, 2}));
        first->invoke([&]() -> void {
            REQUIRE_THROWS_AS(el::LogicFailure, pool->events());
            REQUIRE_THROWS_AS(el::LogicFailure, second->get<el::Utilities>().createWorkerPool());
        });
        static_cast<void>(first->runUntilIdle());
        REQUIRE_FALSE(first->hasError());
        REQUIRE_FALSE(second->hasError());
    }

    void testIndependentPoolsAndConfiguredAdmission() {
        const auto loop = el::EventLoop::create();
        auto first = el::WorkerPoolPtr{};
        auto second = el::WorkerPoolPtr{};
        loop->invoke([&]() -> void {
            auto &utilities = loop->get<el::Utilities>();
            const auto options = el::WorkerPoolOptions{}.setWorkerCount(1).setMaximumPendingTasks(1);
            first = utilities.createWorkerPool(options);
            second = utilities.createWorkerPool(options);
        });
        REQUIRE(loop->runOnce());
        auto release = std::latch{1};
        auto work = first->submit([&]() -> void { release.wait(); });
        // Release before assertions so a regression cannot strand the pool destructor.
        auto rejected = false;
        try {
            auto unexpected = first->submit([]() -> void {});
        } catch (const el::WorkerPoolFullError &) {
            rejected = true;
        }
        const auto result = second->submit([]() -> int { return 7; }).get();
        release.count_down();
        work.get();
        first->stop();
        REQUIRE(rejected);
        REQUIRE_EQUAL(result, 7);
        REQUIRE_FALSE(second->stopToken().isStopRequested());
        REQUIRE_EQUAL(second->submit([]() -> int { return 9; }).get(), 9);
    }

    void testFactoryDoesNotRetainSources() {
        const auto loop = el::EventLoop::create();
        auto weak = std::weak_ptr<el::WorkerPool>{};
        auto called = false;
        loop->invoke([&]() -> void {
            const auto pool = loop->get<el::Utilities>().createWorkerPool(el::WorkerPoolOptions{}.setWorkerCount(1));
            weak = pool;
            pool->submit([]() -> void {}, [&]() -> void { called = true; });
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE(weak.expired());
        REQUIRE_FALSE(called);
        REQUIRE_FALSE(loop->hasError());
    }
};
