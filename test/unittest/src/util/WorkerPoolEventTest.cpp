// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CurrentEvents.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/WorkerCancelledError.hpp>
#include <erbsland/WorkerPool.hpp>
#include <erbsland/WorkerPoolClosedError.hpp>
#include <erbsland/WorkerPoolFullError.hpp>

#include <atomic>
#include <latch>
#include <memory>
#include <ranges>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(WorkerPool WorkerPoolEventEditor WorkerPoolState WorkerPoolWake WorkerDelivery)
class WorkerPoolEventTest final : public el::UnitTest {
public:
    void testTypedResultsAffinityAndFinalOrdering() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
        auto calls = std::vector<int>{};
        auto result = std::vector<int>{};
        loop->invoke([&]() -> void {
            auto &editor = pool->events();
            REQUIRE_EQUAL(&editor, &pool->events());
            REQUIRE_EQUAL(editor.source(), pool);
            REQUIRE_EQUAL(editor.target(), loop);
            editor.onIdle([&]() -> void { calls.push_back(3); }).onFinal([&]() -> void { calls.push_back(4); });
            pool->submit(
                []() -> std::unique_ptr<int> { return std::make_unique<int>(5); },
                [&, capture = std::make_unique<int>(7)](std::unique_ptr<int> value) -> void {
                    REQUIRE_EQUAL(el::currentEvents(), loop);
                    REQUIRE_EQUAL(*value + *capture, 12);
                    calls.push_back(1);
                });
            pool->map(
                std::views::iota(1, 4),
                [](const int &value) -> int { return value * 2; },
                [&](el::List<int> values) -> void {
                    result = values.toStdVector();
                    calls.push_back(2);
                });
            pool->quit();
            pool->join();
            REQUIRE(calls.empty());
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(result, (std::vector<int>{2, 4, 6}));
        REQUIRE_EQUAL(calls.size(), std::size_t{4});
        REQUIRE_EQUAL(calls[2], 3);
        REQUIRE_EQUAL(calls[3], 4);
    }

    void testErrorPrecedenceFallbackAndThrowingCallbackRearming() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto individual = 0;
        auto fallback = 0;
        auto final = false;
        loop->invoke([&]() -> void {
            pool->events()
                .onError([&](std::exception_ptr error) -> void {
                    REQUIRE_THROWS_AS(el::NotFoundError, std::rethrow_exception(error));
                    ++fallback;
                })
                .onFinal([&]() -> void { final = true; });
            const auto fail = []() -> void { throw el::NotFoundError{"Missing."_el}; };
            pool->submit(
                fail,
                []() -> void {},
                [&](std::exception_ptr error) -> void {
                    REQUIRE_THROWS_AS(el::NotFoundError, std::rethrow_exception(error));
                    ++individual;
                });
            pool->submit(fail, []() -> void {});
            pool->submit([]() -> void {}, []() -> void { throw el::NotFoundError{"Callback failed."_el}; });
            pool->quit();
            pool->join();
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(individual, 1);
        REQUIRE_EQUAL(fallback, 1);
        REQUIRE(final);
        REQUIRE(loop->hasError());
        REQUIRE_THROWS_AS(el::NotFoundError, std::rethrow_exception(loop->takeError()));
    }

    void testUnhandledTaskErrorUsesLoopPolicy() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        loop->invoke([&]() -> void {
            pool->submit([]() -> int { throw el::NotFoundError{"Missing."_el}; }, [](int) -> void {});
            pool->quit();
            pool->join();
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE(loop->hasError());
        REQUIRE_THROWS_AS(el::NotFoundError, std::rethrow_exception(loop->takeError()));
    }

    void testReservedDeliveryBypassesFullOrdinaryQueue() {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setMaximumQueuedEvents(1));
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto delivered = 0;
        auto ordinary = 0;
        loop->invoke([&]() -> void {
            loop->invoke([&]() -> void { ++ordinary; });
            for (auto index = 0; index < 1100; ++index) {
                pool->submit([]() -> void {}, [&]() -> void { ++delivered; });
            }
            pool->quit();
            pool->join();
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(delivered, 1100);
        REQUIRE_EQUAL(ordinary, 1);
    }

    void testEventAdmissionRetainedUntilDispatchAndReentrantSubmission() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1).setMaximumPendingTasks(1));
        auto calls = 0;
        loop->invoke([&]() -> void {
            pool->submit(
                []() -> void {},
                [&]() -> void {
                    ++calls;
                    pool->submit([]() -> void {}, [&]() -> void { ++calls; });
                    pool->quit();
                    pool->join();
                });
            pool->wait();
            REQUIRE_EQUAL(calls, 0);
            REQUIRE_THROWS_AS(el::WorkerPoolFullError, pool->submit([]() -> void {}));
        });
        REQUIRE(loop->runOnce());
        pool->wait();
        // A bounded run waits for the worker to finish posting its reserved wake.
        REQUIRE(loop->runOnce(el::TimeDelta::seconds(1)));
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(calls, 2);
    }

    void testCancellationRoutesAndCommittedSuccessSurvivesStop() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto cancelled = 0;
        auto individual = 0;
        auto success = 0;
        auto started = std::latch{1};
        auto release = std::latch{1};
        loop->invoke([&]() -> void {
            pool->events().onCancelled([&]() -> void { ++cancelled; });
            pool->submit([]() -> void {}, [&]() -> void { ++success; });
            pool->wait();
            pool->submit(
                [&]() -> void {
                    started.count_down();
                    release.wait();
                },
                []() -> void {});
            started.wait();
            pool->submit(
                []() -> void {},
                []() -> void {},
                [&](std::exception_ptr error) -> void {
                    REQUIRE_THROWS_AS(el::WorkerCancelledError, std::rethrow_exception(error));
                    ++individual;
                });
            pool->stop();
            release.count_down();
            pool->join();
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(success, 1);
        REQUIRE_EQUAL(cancelled, 1);
        REQUIRE_EQUAL(individual, 1);
    }

    void testDestructionSuppressesQueuedCallbacks() {
        const auto loop = el::EventLoop::create();
        auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto called = false;
        loop->invoke([&]() -> void {
            pool->submit([]() -> int { return 1; }, [&](int) -> void { called = true; });
            pool.reset();
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_FALSE(called);
    }

    void testOwnerQuitDiscardsOutcomesAndReleasesAdmission() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1).setMaximumPendingTasks(1));
        auto release = std::latch{1};
        auto capture = std::make_shared<int>(1);
        const auto weak = std::weak_ptr<int>{capture};
        auto called = false;
        loop->invoke([&]() -> void {
            pool->submit(
                [&]() -> void { release.wait(); }, [owned = std::move(capture), &called]() -> void { called = true; });
            loop->quit();
        });
        static_cast<void>(loop->runUntilIdle());
        release.count_down();
        pool->wait();
        // join also waits for the attempted completion wake and its discard guard.
        pool->quit();
        pool->join();
        REQUIRE_FALSE(called);
        REQUIRE(weak.expired());
    }

    void testOwnerChecksAndDeferredEmptyBulkCompletion() {
        const auto first = el::EventLoop::create();
        const auto second = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto calls = 0;
        first->invoke([&]() -> void {
            pool->forEach(std::vector<int>{}, [](const int &) -> void {}, [&]() -> void { ++calls; });
            pool->wait();
            REQUIRE_EQUAL(calls, 0);
        });
        REQUIRE(first->runOnce());
        second->invoke([&]() -> void {
            REQUIRE_THROWS_AS(el::LogicFailure, pool->events());
            REQUIRE_THROWS_AS(el::LogicFailure, pool->submit([]() -> void {}, []() -> void {}));
        });
        REQUIRE(second->runOnce());
        pool->quit();
        pool->join();
        static_cast<void>(first->runUntilIdle());
        REQUIRE_EQUAL(calls, 1);
    }
    void testNewWorkSuppressesStaleIdleNotification() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto idle = 0;
        auto first = 0;
        auto release = std::latch{1};
        loop->invoke([&]() -> void {
            pool->events().onIdle([&]() -> void { ++idle; });
            pool->submit([]() -> void {}, [&]() -> void { ++first; });
            pool->wait();
        });
        REQUIRE(loop->runOnce());
        REQUIRE(loop->runOnce(el::TimeDelta::seconds(1)));
        REQUIRE_EQUAL(first, 1);
        auto pending = pool->submit([&]() -> void { release.wait(); });
        static_cast<void>(loop->runUntilIdle());
        const auto beforeRelease = idle;
        release.count_down();
        pool->quit();
        pool->join();
        pending.get();
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(beforeRelease, 0);
        REQUIRE_EQUAL(idle, 1);
    }

    void testClosedOwnerReleasesCapacityForProceduralWork() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1).setMaximumPendingTasks(2));
        auto release = std::latch{1};
        auto called = false;
        loop->invoke([&]() -> void {
            pool->submit([&]() -> void { release.wait(); }, [&]() -> void { called = true; });
            loop->quit();
            REQUIRE_THROWS_AS(el::WorkerPoolClosedError, pool->submit([]() -> void {}, []() -> void {}));
        });
        static_cast<void>(loop->runUntilIdle());
        auto sentinel = pool->submit([]() -> void {});
        release.count_down();
        sentinel.get();
        REQUIRE_EQUAL(pool->submit([]() -> int { return 4; }).get(), 4);
        REQUIRE_FALSE(called);
    }
    void testOwnerQuitSuppressesAlreadyQueuedOutcomes() {
        const auto loop = el::EventLoop::create();
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto calls = 0;
        auto capture = std::make_shared<int>(1);
        const auto weak = std::weak_ptr<int>{capture};
        loop->invoke([&]() -> void {
            pool->submit([]() -> void {}, [owned = std::move(capture), &calls]() -> void { ++calls; });
            pool->quit();
            pool->join();
            loop->quit();
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(calls, 0);
        REQUIRE(weak.expired());
    }
};
