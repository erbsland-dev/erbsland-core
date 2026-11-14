// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EpollEventLoopTestCalls.hpp"

#include <erbsland/event/impl/EpollEventLoopDriver.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/system/impl/PosixErrorContext.hpp>
#include <erbsland/system/PlatformFailure.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <limits>

using namespace el::time;

TESTED_TARGETS(EpollEventLoopDriver)
class EpollEventLoopDriverTest final : public el::UnitTest {
public:
    void testShortWakeWritePropagatesPlatformFailure() {
        const auto calls = std::make_shared<EpollEventLoopTestCalls>();
        auto driver = el::event::impl::EpollEventLoopDriver{calls};
        calls->shortWrite = true;
        REQUIRE_THROWS_AS(el::system::PlatformFailure, driver.wake());
    }

    void testInvalidDescriptorReportsNativeContext() {
        auto driver = el::event::impl::EpollEventLoopDriver{};
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

    void testDescriptorReadinessAndRegistrationGeneration() {
        auto driver = el::event::impl::EpollEventLoopDriver{};
        auto descriptors = std::array<int, 2>{};
        REQUIRE_EQUAL(::pipe(descriptors.data()), 0);
        auto oldCalls = 0;
        auto newCalls = 0;
        auto oldRegistration = driver.registerDescriptor(
            descriptors[0],
            true,
            false,
            [&oldCalls]([[maybe_unused]] bool readable, [[maybe_unused]] bool writable, [[maybe_unused]] bool error)
                -> void { oldCalls += 1; });
        oldRegistration.reset();
        auto newRegistration = driver.registerDescriptor(
            descriptors[0],
            true,
            false,
            [&newCalls](const bool readable, [[maybe_unused]] bool writable, [[maybe_unused]] bool error) -> void {
                if (readable) {
                    newCalls += 1;
                }
            });

        constexpr auto byte = char{'x'};
        REQUIRE_EQUAL(::write(descriptors[1], &byte, sizeof(byte)), static_cast<ssize_t>(sizeof(byte)));
        driver.wait(TimeDelta::zero());
        REQUIRE_EQUAL(oldCalls, 0);
        REQUIRE_EQUAL(newCalls, 1);

        newRegistration.reset();
        driver.wait(TimeDelta::zero());
        REQUIRE_EQUAL(newCalls, 1);
        REQUIRE_EQUAL(::close(descriptors[0]), 0);
        REQUIRE_EQUAL(::close(descriptors[1]), 0);
    }

    void testInterruptedWakeAndRemoval() {
        const auto calls = std::make_shared<EpollEventLoopTestCalls>();
        auto driver = el::event::impl::EpollEventLoopDriver{calls};
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
        REQUIRE_EQUAL(calls->removalCalls, 3);
        // Re-registering the same descriptor proves both old filters were removed.
        registration = driver.registerDescriptor(descriptors[0], true, true, [](bool, bool, bool) -> void {});
        calls->removalError = 0;
        registration.reset();
        REQUIRE_EQUAL(::close(descriptors[0]), 0);
        REQUIRE_EQUAL(::close(descriptors[1]), 0);
    }

    void testWakeSaturationAndDrainErrors() {
        const auto calls = std::make_shared<EpollEventLoopTestCalls>();
        auto driver = el::event::impl::EpollEventLoopDriver{calls};
        driver.wake();
        calls->wakeError = EAGAIN;
        driver.wake();
        calls->readInterrupts = 2;
        driver.wait(el::time::TimeDelta::zero());
        REQUIRE_EQUAL(calls->readInterrupts, 0);
        calls->wakeError = 0;
        driver.wake();
        calls->readError = EBADF;
        try {
            driver.wait(el::time::TimeDelta::zero());
            REQUIRE(false);
        } catch (const el::PlatformError &error) {
            const auto context = std::dynamic_pointer_cast<const el::system::impl::PosixErrorContext>(error.context());
            REQUIRE(context != nullptr);
            REQUIRE_EQUAL(context->errorCode(), EBADF);
        }
        calls->readError = 0;
        driver.wait(el::time::TimeDelta::zero());
    }
};
