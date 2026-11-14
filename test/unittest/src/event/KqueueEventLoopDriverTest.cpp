// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "KqueueEventLoopTestCalls.hpp"

#include <erbsland/event/impl/KqueueEventLoopDriver.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/system/impl/PosixErrorContext.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <limits>

TESTED_TARGETS(KqueueEventLoopDriver)
class KqueueEventLoopDriverTest final : public el::UnitTest {
public:
    void testInvalidDescriptorReportsNativeContext() {
        auto driver = el::event::impl::KqueueEventLoopDriver{};
        try {
            const auto registration = driver.registerDescriptor(
                std::numeric_limits<int>::max(), true, false, [](bool, bool, bool) -> void {});
            REQUIRE(false);
        } catch (const el::PlatformError &error) {
            const auto context = std::dynamic_pointer_cast<const el::system::impl::PosixErrorContext>(error.context());
            REQUIRE(context != nullptr);
            REQUIRE_EQUAL(context->errorCode(), EBADF);
        }
    }

    void testConstruction() {
        const auto driver = el::event::impl::KqueueEventLoopDriver{};
        static_cast<void>(driver);
    }

    void testInterruptedWakeAndRemoval() {
        const auto calls = std::make_shared<KqueueEventLoopTestCalls>();
        auto driver = el::event::impl::KqueueEventLoopDriver{calls};
        calls->wakeInterrupts = 2;
        driver.wake();
        REQUIRE_EQUAL(calls->wakeCalls, 3);
        driver.wait(el::time::TimeDelta::zero());
        int descriptors[2]{};
        REQUIRE_EQUAL(::socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors), 0);
        auto registration = driver.registerDescriptor(descriptors[0], true, true, [](bool, bool, bool) -> void {});
        calls->removalInterrupts = 2;
        calls->removalError = ENOENT;
        registration.reset();
        REQUIRE_EQUAL(calls->removalCalls, 4);
        // Re-registering the same descriptor proves both old filters were removed.
        registration = driver.registerDescriptor(descriptors[0], true, true, [](bool, bool, bool) -> void {});
        calls->removalError = 0;
        registration.reset();
        REQUIRE_EQUAL(::close(descriptors[0]), 0);
        REQUIRE_EQUAL(::close(descriptors[1]), 0);
    }
};
