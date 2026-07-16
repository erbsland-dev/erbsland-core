// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/CoGenerator.hpp>

#include <concepts>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using el::util::CoGenerator;

TESTED_TARGETS(CoGenerator)
class CoGeneratorTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        using IntGenerator = CoGenerator<int>;

        static_assert(!std::copy_constructible<IntGenerator>);
        static_assert(!std::is_copy_assignable_v<IntGenerator>);
        static_assert(std::move_constructible<IntGenerator>);
        static_assert(std::is_move_assignable_v<IntGenerator>);
        static_assert(std::ranges::input_range<IntGenerator &>);
        static_assert(std::same_as<std::ranges::range_value_t<IntGenerator &>, int>);
        static_assert(std::same_as<IntGenerator::iterator::reference, const int &>);
    }

    void testNextConsumption() {
        auto generator = numbers(3);

        auto value = generator.next();
        REQUIRE(value.has_value());
        REQUIRE_EQUAL(*value, 0);

        value = generator.next();
        REQUIRE(value.has_value());
        REQUIRE_EQUAL(*value, 1);

        value = generator.next();
        REQUIRE(value.has_value());
        REQUIRE_EQUAL(*value, 2);

        REQUIRE_FALSE(generator.next().has_value());
        REQUIRE_FALSE(generator.next().has_value());
    }

    void testRangeIteration() {
        auto values = std::vector<int>{};
        for (auto value : numbers(4)) {
            values.push_back(value);
        }

        REQUIRE_EQUAL(values, (std::vector<int>{0, 1, 2, 3}));
    }

    void testEmptyGenerator() {
        auto generator = emptyNumbers();

        REQUIRE_FALSE(generator.next().has_value());

        auto values = std::vector<int>{};
        for (auto value : emptyNumbers()) {
            values.push_back(value);
        }
        REQUIRE(values.empty());
    }

    void testStringValues() {
        auto values = std::vector<std::string>{};
        for (const auto &value : strings()) {
            values.push_back(value);
        }

        REQUIRE_EQUAL(values, (std::vector<std::string>{"alpha", "beta", "gamma"}));
    }

    void testMoveOnlyValuesWithNext() {
        auto generator = uniqueNumbers();

        auto first = generator.next();
        REQUIRE(first.has_value());
        REQUIRE(*first != nullptr);
        REQUIRE_EQUAL(**first, 4);

        auto second = generator.next();
        REQUIRE(second.has_value());
        REQUIRE(*second != nullptr);
        REQUIRE_EQUAL(**second, 5);

        REQUIRE_FALSE(generator.next().has_value());
    }

    void testMoveOwnership() {
        auto generator = numbers(2);
        auto moved = std::move(generator);

        auto first = moved.next();
        REQUIRE(first.has_value());
        REQUIRE_EQUAL(*first, 0);

        auto second = moved.next();
        REQUIRE(second.has_value());
        REQUIRE_EQUAL(*second, 1);

        REQUIRE_FALSE(moved.next().has_value());
        REQUIRE_FALSE(generator.next().has_value());
    }

    void testExceptionPropagationWithNext() {
        auto generator = valuesBeforeFailure();

        auto value = generator.next();
        REQUIRE(value.has_value());
        REQUIRE_EQUAL(*value, 1);

        value = generator.next();
        REQUIRE(value.has_value());
        REQUIRE_EQUAL(*value, 2);

        REQUIRE_THROWS(generator.next());
        REQUIRE_THROWS(generator.next());
    }

    void testExceptionPropagationWithRangeIteration() {
        auto values = std::vector<int>{};

        REQUIRE_THROWS(([&]() -> void {
            auto generator = valuesBeforeFailure();
            for (auto value : generator) {
                values.push_back(value);
            }
        }()));
        REQUIRE_EQUAL(values, (std::vector<int>{1, 2}));
    }

    void testCopiedIteratorAfterCompletion() {
        auto generator = numbers(1);
        auto iterator = generator.begin();
        auto copy = iterator;

        ++iterator;
        REQUIRE(iterator == generator.end());

        ++copy;
        REQUIRE(copy == generator.end());
    }

    void testInvalidIteratorAccessThrows() {
        auto iterator = CoGenerator<int>::iterator{};

        REQUIRE_THROWS(*iterator);
        REQUIRE_THROWS(++iterator);
    }

private:
    [[nodiscard]] static auto numbers(int count) -> CoGenerator<int> {
        for (auto value = 0; value < count; ++value) {
            co_yield value;
        }
    }

    [[nodiscard]] static auto emptyNumbers() -> CoGenerator<int> {
        if (false) {
            co_yield 0;
        }
    }

    [[nodiscard]] static auto strings() -> CoGenerator<std::string> {
        co_yield "alpha";
        auto beta = std::string{"beta"};
        co_yield beta;
        co_yield std::string{"gamma"};
    }

    [[nodiscard]] static auto uniqueNumbers() -> CoGenerator<std::unique_ptr<int>> {
        co_yield std::make_unique<int>(4);
        co_yield std::make_unique<int>(5);
    }

    [[nodiscard]] static auto valuesBeforeFailure() -> CoGenerator<int> {
        co_yield 1;
        co_yield 2;
        throw std::runtime_error{"test failure"};
    }
};
