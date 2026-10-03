// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/StopSource.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

TESTED_TARGETS(StopSource StopToken)
class StopSourceTest final : public el::UnitTest {
public:
    void testDefaultAndSurvivingTokens() {
        REQUIRE_FALSE(el::StopToken{}.isStopRequested());
        auto token = el::StopToken{};
        {
            auto source = el::StopSource{};
            token = source.stopToken();
            REQUIRE_FALSE(token.isStopRequested());
        }
        REQUIRE_FALSE(token.isStopRequested());
        {
            auto source = el::StopSource{};
            token = source.stopToken();
            REQUIRE(source.requestStop());
            REQUIRE_FALSE(source.requestStop());
        }
        REQUIRE(token.isStopRequested());
    }
    void testCopiesSignalOnceAcrossThreads() {
        const auto source = el::StopSource{};
        const auto token = source.stopToken();
        auto winners = std::atomic<unsigned>{0};
        auto threads = std::vector<std::thread>{};
        for (auto index = 0; index < 8; ++index) {
            threads.emplace_back([copy = source, &winners]() mutable -> void {
                if (copy.requestStop()) {
                    ++winners;
                }
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }
        REQUIRE_EQUAL(winners.load(), 1U);
        REQUIRE(source.isStopRequested());
        REQUIRE(token.isStopRequested());
    }
};
