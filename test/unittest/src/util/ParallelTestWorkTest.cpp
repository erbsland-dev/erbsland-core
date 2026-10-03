// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ParallelTestWork.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>

TESTED_TARGETS(ParallelTestWork)
class ParallelTestWorkTest final : public el::UnitTest {
public:
    void testOrderedResultsAndJoinedWorkers() {
        auto visits = std::array<std::atomic<unsigned>, 128U>{};
        const auto caller = std::this_thread::get_id();
        const auto results = parallel_test_work::run(visits.size(), [&](const std::size_t index) -> std::size_t {
            ++visits[index];
            if (std::this_thread::get_id() == caller) {
                throw std::runtime_error{"Computation ran in the assertion thread."};
            }
            return index * index;
        });
        REQUIRE_EQUAL(results.size(), visits.size());
        for (auto index = std::size_t{}; index < results.size(); ++index) {
            REQUIRE_EQUAL(visits[index].load(), 1U);
            REQUIRE_EQUAL(results[index].value(), index * index);
        }
    }
    void testFailureIsDeferredAndOtherCasesFinish() {
        const auto results = parallel_test_work::run(4U, [](const std::size_t index) -> std::unique_ptr<std::size_t> {
            if (index == 1U) {
                throw std::runtime_error{"Independent computation failed."};
            }
            if (index == 2U) {
                throw 42;
            }
            return std::make_unique<std::size_t>(index);
        });
        REQUIRE_EQUAL(*results[0].value(), 0U);
        runWithContext(SOURCE_LOCATION(), [&]() -> void { REQUIRE_THROWS_AS(std::runtime_error, results[1].value()); });
        REQUIRE_THROWS_AS(int, results[2].value());
        REQUIRE_EQUAL(*results[3].value(), 3U);
    }
    void testEmptyWorkAndSingleWorker() {
        const auto compute = [](const std::size_t index) -> std::size_t { return index; };
        REQUIRE(parallel_test_work::run(0U, compute).empty());
        const auto results = parallel_test_work::run(8U, compute, 1U);
        for (auto index = std::size_t{}; index < results.size(); ++index) {
            REQUIRE_EQUAL(results[index].value(), index);
        }
    }
};
