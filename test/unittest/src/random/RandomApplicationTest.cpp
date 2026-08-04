// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/random/SecureRandom.hpp>
#include <erbsland/random/ThreadSafeFastRandom.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

using el::core::Application;

TESTED_TARGETS(Application FastRandom SecureRandom)
class RandomApplicationTest final : public el::UnitTest {
public:
    void testStableAccessors() {
        auto scope = ApplicationTestScope<Application>{};
        auto &application = scope.app();

        const auto *random = &application.random();
        const auto *secureRandom = &application.secureRandom();
        const auto randomValue = application.random().getInt32(1, 1);
        REQUIRE_EQUAL(random, &application.random());
        REQUIRE_EQUAL(secureRandom, &application.secureRandom());
        REQUIRE_EQUAL(randomValue, 1);
    }

    void testConcurrentApplicationRandom() {
        auto scope = ApplicationTestScope<Application>{};
        auto &application = scope.app();
        auto failures = std::atomic<int>{0};
        auto threads = std::vector<std::thread>{};

        for (auto threadIndex = 0; threadIndex < 8; ++threadIndex) {
            threads.emplace_back([&]() -> void {
                for (auto i = 0; i < 100; ++i) {
                    const auto value = application.random().getUInt32(0U, 5U);
                    if (value > 5U) {
                        ++failures;
                    }
                }
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }
        REQUIRE_EQUAL(failures.load(), 0);
    }
};
