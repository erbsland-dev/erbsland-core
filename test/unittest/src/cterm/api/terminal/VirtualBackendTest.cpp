// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestHelper.hpp"

#include <erbsland/cterm/impl/VirtualBackend.hpp>

TESTED_TARGETS(VirtualBackend Terminal Backend)
class VirtualBackendTest final : public UNITTEST_SUBCLASS(TerminalTestHelper) {
public:
    void testIndependentLifecycleAndCapabilities() {
        const auto first = Backend::createPlatformDefault(TerminalFlag::VirtualTty);
        const auto second = Backend::createPlatformDefault(TerminalFlag::VirtualTty | TerminalFlag::NoSignalHandling);
        REQUIRE(first != second);
        REQUIRE_FALSE(first->isInteractive());
        first->initializePlatform();
        REQUIRE(first->isInteractive());
        REQUIRE_FALSE(second->isInteractive());
        REQUIRE(first->supportsColorCodes());
        REQUIRE(first->supportsCursorCodes());
        REQUIRE(first->supportsCursorVisibilityCodes());
        REQUIRE(first->supportsAlternateScreenBufferCodes());
        REQUIRE_FALSE(first->detectScreenSize().has_value());
        first->restorePlatform();
        REQUIRE_FALSE(first->isInteractive());
    }

    void testManualSizeSurvivesDetectionAndClamps() {
        auto terminal = Terminal{block::Size{90, 40}, TerminalFlag::VirtualTty};
        terminal.testScreenSize();
        REQUIRE_EQUAL(terminal.size(), block::Size(90, 40));
        terminal.setSize({120, 50});
        terminal.testScreenSize();
        REQUIRE_EQUAL(terminal.size(), block::Size(120, 50));
        terminal.setSize({0, 5000});
        REQUIRE_EQUAL(terminal.size(), block::Size(1, 2048));
    }

    void testInputIsEmptyAndKeyPollKeepsItsDelay() {
        auto backend = el::cterm::impl::VirtualBackend{};
        REQUIRE(backend.readLine().isEmpty());
        REQUIRE_FALSE(backend.waitForKey().valid());
        REQUIRE_FALSE(backend.readKey(el::Milliseconds{1000}).valid()); // Line mode never waits.
        backend.setInputMode(Input::Mode::Key);
        REQUIRE_EQUAL(backend.inputMode(), Input::Mode::Key);
        REQUIRE_FALSE(backend.readKey(el::Milliseconds{-1}).valid());
        REQUIRE_FALSE(backend.readKey(el::Milliseconds{}).valid());
        const auto started = std::chrono::steady_clock::now();
        REQUIRE_FALSE(backend.readKey(el::Milliseconds{5}).valid());
        REQUIRE(std::chrono::steady_clock::now() - started >= std::chrono::milliseconds{5});
        REQUIRE_FALSE(backend.waitForKey().valid());
    }
};
