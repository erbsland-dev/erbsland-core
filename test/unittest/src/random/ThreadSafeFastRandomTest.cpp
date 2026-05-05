// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/random/ThreadSafeFastRandom.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

using el::random::ThreadSafeFastRandom;

TESTED_TARGETS(ThreadSafeFastRandom)
class ThreadSafeFastRandomTest final : public el::UnitTest {
public:
    void testConcurrentAccess() {
        auto random = ThreadSafeFastRandom{123U};
        auto failures = std::atomic<int>{0};
        auto threads = std::vector<std::thread>{};

        for (auto threadIndex = 0; threadIndex < 8; ++threadIndex) {
            threads.emplace_back([&]() -> void {
                for (auto i = 0; i < 1000; ++i) {
                    const auto value = random.getInt32(-10, 10);
                    if (value < -10 || value > 10) {
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
