// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <csignal>
#include <thread>

class SignalApplication final : public el::core::Application {
protected:
    void initialize() override {
        _signalThread = std::thread{[]() -> void {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            std::raise(SIGTERM);
        }};
    }

    void cleanup() noexcept override {
        if (_signalThread.joinable()) {
            _signalThread.join();
        }
    }

private:
    std::thread _signalThread;
};

TESTED_TARGETS(Application ProcessSignalDispatcher)
class PosixApplicationServiceLifecycleTest final : public el::UnitTest {
public:
    void testSigtermRequestsGracefulShutdown() {
        auto scope = ApplicationTestScope<SignalApplication>{};
        auto &application = scope.app();
        application.enableServiceLifecycle();
        REQUIRE_EQUAL(application.run(), 0);
    }
};
