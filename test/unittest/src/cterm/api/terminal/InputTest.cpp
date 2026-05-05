// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InputTestProbe.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>

TESTED_TARGETS(Input)
class InputTest final : public el::UnitTest {
public:
    void testModeAndReadLineUseTheImplementationHooks() {
        auto input = InputProbe{};

        REQUIRE_EQUAL(input.mode(), Input::Mode::ReadLine);
        REQUIRE_EQUAL(input.readLine(), "typed line"_el);

        input.setMode(Input::Mode::Key);

        REQUIRE_EQUAL(input.mode(), Input::Mode::Key);
    }

    void testReadKeyClampsNegativeTimeoutsToZero() {
        const auto input = InputProbe{};

        const auto key = input.readKey(std::chrono::milliseconds{-25});

        REQUIRE_EQUAL(key, Key::Escape);
        REQUIRE_EQUAL(input._lastTimeout, std::chrono::milliseconds{0});
        REQUIRE_EQUAL(input._readKeyCallCount, 1);
    }

    void testReadKeyPassesThroughPositiveTimeouts() {
        const auto input = InputProbe{};

        const auto key = input.readKey(std::chrono::milliseconds{125});

        REQUIRE_EQUAL(key, Key::Escape);
        REQUIRE_EQUAL(input._lastTimeout, std::chrono::milliseconds{125});
        REQUIRE_EQUAL(input._readKeyCallCount, 1);
    }

    void testWaitForKeyUsesTheBlockingHook() {
        const auto input = InputProbe{};

        const auto key = input.waitForKey();

        REQUIRE_EQUAL(key, Key::Enter);
        REQUIRE_EQUAL(input._waitForKeyCallCount, 1);
        REQUIRE_EQUAL(input._readKeyCallCount, 0);
    }
};
