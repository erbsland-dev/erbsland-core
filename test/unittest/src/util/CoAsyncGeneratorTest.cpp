// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/CoAsyncGenerator.hpp>
#include <erbsland/util/CoTask.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

using el::util::CoAsyncGenerator;
using el::util::CoTask;

TESTED_TARGETS(CoAsyncGenerator)
class CoAsyncGeneratorTest final : public el::UnitTest {
public:
    void testLazyGenerationAndAsyncAwait() {
        auto steps = std::atomic<int>{0};
        auto generator = numbers(steps);
        REQUIRE_EQUAL(steps.load(), 0);

        auto task = collectNumbers(std::move(generator));
        waitFor(task);
        REQUIRE(task.isComplete());
        REQUIRE_EQUAL(task.takeResult(), (std::vector<int>{1, 2, 3}));
        REQUIRE_EQUAL(steps.load(), 2);
    }

    void testMoveOnlyValues() {
        auto generator = uniqueNumbers();

        auto first = nextValue(generator);
        waitFor(first);
        auto firstValue = first.takeResult();
        REQUIRE(firstValue.has_value());
        REQUIRE(*firstValue != nullptr);
        REQUIRE_EQUAL(**firstValue, 4);

        auto second = nextValue(generator);
        waitFor(second);
        auto secondValue = second.takeResult();
        REQUIRE(secondValue.has_value());
        REQUIRE(*secondValue != nullptr);
        REQUIRE_EQUAL(**secondValue, 5);

        auto finished = nextValue(generator);
        waitFor(finished);
        REQUIRE_FALSE(finished.takeResult().has_value());
    }

    void testExceptionPropagation() {
        auto generator = failingNumbers();
        auto first = nextValue(generator);
        waitFor(first);
        REQUIRE_EQUAL(*first.takeResult(), 1);

        auto failure = nextValue(generator);
        waitFor(failure);
        REQUIRE_THROWS_AS(std::runtime_error, failure.result());
    }

    void testRejectsConcurrentNext() {
        auto release = std::atomic<bool>{false};
        auto generator = delayedNumber(release);
        auto first = nextValue(generator);
        auto second = nextValue(generator);

        waitFor(second);
        REQUIRE(second.isComplete());
        REQUIRE_THROWS_AS(el::err::LogicError, second.result());

        release.store(true);
        waitFor(first);
        REQUIRE_EQUAL(*first.takeResult(), 7);
    }

private:
    template <typename T>
    static void waitFor(CoTask<T> &task) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!task.isComplete() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
    }

    [[nodiscard]] static auto numbers(std::atomic<int> &steps) -> CoAsyncGenerator<int> {
        steps.fetch_add(1);
        co_yield 1;
        const auto value = co_await CoTask<int>::run([]() -> int { return 2; });
        steps.fetch_add(1);
        co_yield value;
        co_yield 3;
    }

    [[nodiscard]] static auto uniqueNumbers() -> CoAsyncGenerator<std::unique_ptr<int>> {
        co_yield std::make_unique<int>(4);
        co_yield std::make_unique<int>(5);
    }

    [[nodiscard]] static auto failingNumbers() -> CoAsyncGenerator<int> {
        co_yield 1;
        throw std::runtime_error{"generator failure"};
    }

    template <typename T>
    [[nodiscard]] static auto nextValue(CoAsyncGenerator<T> &generator) -> CoTask<std::optional<T>> {
        co_return co_await generator.next();
    }

    [[nodiscard]] static auto collectNumbers(CoAsyncGenerator<int> generator) -> CoTask<std::vector<int>> {
        auto result = std::vector<int>{};
        while (auto value = co_await generator.next()) {
            result.push_back(*value);
        }
        co_return result;
    }

    [[nodiscard]] static auto delayedNumber(std::atomic<bool> &release) -> CoAsyncGenerator<int> {
        const auto value = co_await CoTask<int>::run([&release]() -> int {
            while (!release.load()) {
                std::this_thread::yield();
            }
            return 7;
        });
        co_yield value;
    }
};
