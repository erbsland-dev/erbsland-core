// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/LogicFailure.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/WorkerCancelledError.hpp>
#include <erbsland/WorkerPool.hpp>
#include <erbsland/WorkerPoolClosedError.hpp>
#include <erbsland/WorkerPoolFullError.hpp>

#include <atomic>
#include <future>
#include <latch>
#include <memory>
#include <thread>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(WorkerPool WorkerPoolOptions WorkerPoolClosedError WorkerPoolFullError WorkerCancelledError)
class WorkerPoolTest final : public el::UnitTest {
    struct Capture {
        explicit Capture(std::atomic<int> &count) : count{count} {}
        ~Capture() { ++count; }
        std::atomic<int> &count;
    };
    struct BothForms {
        auto operator()() const -> int { return 1; }
        auto operator()(el::StopToken) const -> int { return 2; }
    };

public:
    void testValuesMoveOnlyVoidErrorsAndReuse() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
        REQUIRE_EQUAL(pool->workerCount(), 2U);
        REQUIRE(pool->isAccepting());
        REQUIRE_FALSE(pool->stopToken().isStopRequested());
        auto value = pool->submit(
            [owned = std::make_unique<int>(42)]() mutable -> std::unique_ptr<int> { return std::move(owned); });
        REQUIRE_EQUAL(*value.get(), 42);
        REQUIRE_EQUAL(pool->submit(BothForms{}).get(), 2);
        auto call = pool->submit([]() -> void {});
        call.get();
        auto error = pool->submit([]() -> int { throw el::NotFoundError{"Missing input."_el}; });
        REQUIRE_THROWS_AS(el::NotFoundError, error.get());
        pool->wait();
        REQUIRE_EQUAL(pool->submit([]() -> int { return 7; }).get(), 7);
    }

    void testWorkerLimitAndAdmission() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2).setMaximumPendingTasks(3));
        auto started = std::latch{2};
        auto release = std::latch{1};
        auto active = std::atomic<int>{0};
        auto calls = std::atomic<int>{0};
        auto futures = std::vector<std::future<void>>{};
        for (auto index = 0; index < 2; ++index) {
            futures.push_back(pool->submit([&]() -> void {
                ++active;
                ++calls;
                started.count_down();
                release.wait();
                --active;
            }));
        }
        started.wait();
        const auto observed = active.load();
        futures.push_back(pool->submit([&]() -> void { ++calls; }));
        auto rejected = false;
        try {
            static_cast<void>(pool->submit([]() -> void {}));
        } catch (const el::WorkerPoolFullError &) {
            rejected = true;
        }
        release.count_down();
        pool->wait();
        REQUIRE_EQUAL(observed, 2);
        REQUIRE(rejected);
        REQUIRE_EQUAL(calls.load(), 3);
        for (auto &future : futures) {
            future.get();
        }
        pool->submit([]() -> void {}).get();
    }

    void testStopCancelsQueuedAndCooperativelyRunningWork() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto started = std::latch{1};
        auto queuedCalls = std::atomic<int>{0};
        auto running = pool->submit([&](el::StopToken token) -> int {
            started.count_down();
            while (!token.isStopRequested()) {
                std::this_thread::yield();
            }
            return 1;
        });
        started.wait();
        auto queued = pool->submit([&]() -> int {
            ++queuedCalls;
            return 2;
        });
        pool->quit();
        pool->stop();
        pool->stop();
        pool->join();
        REQUIRE(pool->stopToken().isStopRequested());
        REQUIRE_FALSE(pool->isAccepting());
        REQUIRE_EQUAL(queuedCalls.load(), 0);
        REQUIRE_THROWS_AS(el::WorkerCancelledError, running.get());
        REQUIRE_THROWS_AS(el::WorkerCancelledError, queued.get());
        REQUIRE_THROWS_AS(el::WorkerPoolClosedError, pool->submit([]() -> void {}));
    }

    void testReadyResultSurvivesStopAndCapturesAreReleased() {
        auto destroyed = std::atomic<int>{0};
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto result = pool->submit([capture = std::make_unique<Capture>(destroyed)]() -> int { return 42; });
        result.wait();
        REQUIRE_EQUAL(destroyed.load(), 1);
        pool->stop();
        REQUIRE_EQUAL(result.get(), 42);
    }

    void testDestructionDrainsAndJoins() {
        auto count = std::atomic<int>{0};
        auto destroyed = std::atomic<int>{0};
        auto futures = std::vector<std::future<void>>{};
        {
            const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
            for (auto index = 0; index < 32; ++index) {
                futures.push_back(
                    pool->submit([&, capture = std::make_unique<Capture>(destroyed)]() -> void { ++count; }));
            }
        }
        REQUIRE_EQUAL(count.load(), 32);
        REQUIRE_EQUAL(destroyed.load(), 32);
        for (auto &future : futures) {
            future.get();
        }
    }

    void testConcurrentProducersAndIndependentPools() {
        const auto first = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
        const auto second = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto count = std::atomic<int>{0};
        auto producers = std::vector<std::thread>{};
        for (auto index = 0; index < 4; ++index) {
            producers.emplace_back([&]() -> void {
                for (auto item = 0; item < 25; ++item) {
                    first->submit([&]() -> void { ++count; }).get();
                }
            });
        }
        for (auto &producer : producers) {
            producer.join();
        }
        first->stop();
        REQUIRE_EQUAL(second->submit([]() -> int { return 3; }).get(), 3);
        REQUIRE_EQUAL(count.load(), 100);
    }

    void testAutomaticCountAndIdempotentGracefulShutdown() {
        const auto pool = el::WorkerPool::create();
        REQUIRE_GREATER_EQUAL(pool->workerCount(), 1U);
        pool->wait();
        pool->quit();
        pool->quit();
        pool->join();
        pool->join();
        REQUIRE_FALSE(pool->stopToken().isStopRequested());
    }
    void testReadyFutureReleasesAdmissionImmediately() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2).setMaximumPendingTasks(1));
        for (auto index = 0; index < 100; ++index) {
            REQUIRE_EQUAL(pool->submit([index]() -> int { return index; }).get(), index);
        }
    }

    void testStopRacesWithCompletionWithoutDoublePublication() {
        for (auto index = 0; index < 24; ++index) {
            const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
            auto ready = std::latch{1};
            auto release = std::latch{1};
            auto result = pool->submit([&]() -> int {
                ready.count_down();
                release.wait();
                return 7;
            });
            ready.wait();
            auto stopper = std::thread{[&]() -> void {
                release.wait();
                pool->stop();
            }};
            release.count_down();
            stopper.join();
            pool->join();
            try {
                const auto value = result.get();
                REQUIRE_EQUAL(value, 7);
            } catch (const el::WorkerCancelledError &) {}
        }
    }
};
