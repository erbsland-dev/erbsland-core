// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/CoTask.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>

using el::util::CoTask;

namespace erbsland::test::cotasktest {

template <typename T>
void waitFor(CoTask<T> &task) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!task.isComplete() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
}

[[nodiscard]] static auto chainedTask() -> CoTask<int> {
    const auto first = co_await CoTask<int>::run([]() -> int { return 20; });
    const auto second = co_await CoTask<int>::run([first]() -> int { return first + 21; });
    co_return second + 1;
}

[[nodiscard]] static auto voidTask(std::atomic<bool> &completed) -> CoTask<void> {
    co_await CoTask<void>::run([&completed]() -> void { completed.store(true); });
}

[[nodiscard]] static auto cancelledTask(
    std::atomic<bool> &release, std::atomic<bool> &workCompleted, std::atomic<bool> &continued) -> CoTask<void> {
    co_await CoTask<void>::run([&release, &workCompleted]() -> void {
        while (!release.load()) {
            std::this_thread::yield();
        }
        workCompleted.store(true);
    });
    continued.store(true);
}

}

using namespace erbsland::test::cotasktest;

TESTED_TARGETS(CoTask CoWorkerService)
class CoTaskTest final : public el::UnitTest {
public:
    void testEagerRunAndResult() {
        auto started = std::atomic<bool>{false};
        auto task = CoTask<int>::run([&started]() -> int {
            started.store(true);
            return 42;
        });

        waitFor(task);
        REQUIRE(started.load());
        REQUIRE(task.isComplete());
        REQUIRE_EQUAL(task.result(), 42);
        REQUIRE_EQUAL(task.takeResult(), 42);
        REQUIRE_THROWS_AS(el::err::LogicError, task.result());
    }

    void testTaskChaining() {
        auto task = chainedTask();

        waitFor(task);
        REQUIRE(task.isComplete());
        REQUIRE_EQUAL(task.takeResult(), 42);
    }

    void testVoidTask() {
        auto completed = std::atomic<bool>{false};
        auto task = voidTask(completed);

        waitFor(task);
        REQUIRE(task.isComplete());
        REQUIRE(completed.load());
        REQUIRE_NOTHROW(task.result());
    }

    void testMoveOnlyResult() {
        auto task =
            CoTask<std::unique_ptr<int>>::run([]() -> std::unique_ptr<int> { return std::make_unique<int>(42); });

        waitFor(task);
        auto result = task.takeResult();
        REQUIRE(result != nullptr);
        REQUIRE_EQUAL(*result, 42);
    }

    void testExceptionPropagation() {
        auto task = CoTask<int>::run([]() -> int { throw std::runtime_error{"task failure"}; });

        waitFor(task);
        REQUIRE(task.isComplete());
        REQUIRE_THROWS_AS(std::runtime_error, task.result());
    }

    void testCancellationByDestruction() {
        auto release = std::atomic<bool>{false};
        auto workCompleted = std::atomic<bool>{false};
        auto continued = std::atomic<bool>{false};
        {
            auto task = cancelledTask(release, workCompleted, continued);
            REQUIRE_FALSE(task.isComplete());
        }
        release.store(true);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!workCompleted.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        REQUIRE(workCompleted.load());
        REQUIRE_FALSE(continued.load());
    }

    void testEmptyAndIncompleteResults() {
        auto empty = CoTask<int>{};
        REQUIRE(empty.isComplete());
        REQUIRE_THROWS_AS(el::err::LogicError, empty.result());

        auto release = std::atomic<bool>{false};
        auto task = CoTask<int>::run([&release]() -> int {
            while (!release.load()) {
                std::this_thread::yield();
            }
            return 1;
        });
        REQUIRE_THROWS_AS(el::err::LogicError, task.result());
        release.store(true);
        waitFor(task);
    }
};
