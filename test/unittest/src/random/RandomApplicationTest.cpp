// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

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
        auto application = Application{};

        REQUIRE(&application.random() == &application.random());
        REQUIRE(&application.secureRandom() == &application.secureRandom());
        REQUIRE(application.random().getInt32(1, 1) == 1);
    }

    void testConcurrentApplicationRandom() {
        auto application = Application{};
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
