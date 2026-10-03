// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/NotFoundError.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/WorkerCancelledError.hpp>
#include <erbsland/WorkerPool.hpp>

#include <atomic>
#include <latch>
#include <ranges>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(WorkerPool WorkerBulkTask)
class WorkerPoolBulkTest final : public el::UnitTest {
public:
    void testOwnedSnapshotOrderedResultsAndOneAdmission() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2).setMaximumPendingTasks(1));
        auto inputs = std::vector<int>{0, 1, 2, 3, 4};
        auto started = std::latch{1};
        auto release = std::latch{1};
        auto mapped = pool->map(inputs | std::views::all, [&](const int &value, el::StopToken) -> int {
            if (value == 0) {
                started.count_down();
                release.wait();
            }
            return value * value;
        });
        started.wait();
        inputs.assign(5, 100);
        release.count_down();
        const auto result = mapped.get();
        REQUIRE_EQUAL(result.toRawValue(), (std::vector<int>{0, 1, 4, 9, 16}));
    }

    void testForEachAndEmptyRanges() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
        auto sum = std::atomic<int>{0};
        pool->forEach(std::views::iota(1, 11), [&](const int &value) -> void { sum += value; }).get();
        REQUIRE_EQUAL(sum.load(), 55);
        auto empty = std::vector<int>{};
        REQUIRE(pool->map(empty, [](const int &value) -> int { return value; }).get().isEmpty());
        pool->forEach(empty, [&](const int &) -> void { ++sum; }).get();
        REQUIRE_EQUAL(sum.load(), 55);
        const auto bits =
            pool->map(std::vector<bool>{true, false}, [](const bool &value) -> int { return int{value}; }).get();
        REQUIRE_EQUAL(bits.toRawValue(), (std::vector<int>{1, 0}));
    }

    void testFailureStopsOnlyItsBatch() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto visited = std::atomic<int>{0};
        auto failed = pool->map(std::views::iota(0, 20), [&](const int &value) -> int {
            ++visited;
            if (value == 2) {
                throw el::NotFoundError{"Missing batch input."_el};
            }
            return value;
        });
        REQUIRE_THROWS_AS(el::NotFoundError, failed.get());
        REQUIRE_EQUAL(visited.load(), 3);
        REQUIRE_EQUAL(pool->submit([]() -> int { return 9; }).get(), 9);
    }

    void testCancellationDrainsRunningItemsWithoutPartialResults() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
        auto started = std::latch{2};
        auto release = std::latch{1};
        auto visited = std::atomic<int>{0};
        auto finished = std::atomic<int>{0};
        auto mapped = pool->map(std::views::iota(0, 100), [&](const int &value) -> int {
            ++visited;
            started.count_down();
            release.wait();
            ++finished;
            return value;
        });
        started.wait();
        pool->stop();
        release.count_down();
        REQUIRE_THROWS_AS(el::WorkerCancelledError, mapped.get());
        REQUIRE_EQUAL(visited.load(), 2);
        REQUIRE_EQUAL(finished.load(), 2);
    }

    void testBulkSchedulingAllowsOtherOperations() {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto started = std::latch{1};
        auto release = std::latch{1};
        auto visited = std::atomic<int>{0};
        auto bulk = pool->forEach(std::views::iota(0, 20), [&](const int &value) -> void {
            if (value == 0) {
                started.count_down();
                release.wait();
            }
            ++visited;
        });
        started.wait();
        auto other = pool->submit([&]() -> int { return visited.load(); });
        release.count_down();
        const auto beforeCompletion = other.get();
        bulk.get();
        REQUIRE_LESS(beforeCompletion, 20);
    }
};
