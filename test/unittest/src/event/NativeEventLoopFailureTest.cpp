// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "NativeEventLoopTestTypes.hpp"

#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/event/impl/EventLoop.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/system/PlatformFailure.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <exception>
#include <thread>

#ifndef ERBSLAND_OS_WINDOWS
#include <sys/socket.h>
#include <unistd.h>
#endif

using namespace el::text::literals;

TESTED_TARGETS(EventLoopDriver PlatformFailure)
class NativeEventLoopFailureTest final : public el::UnitTest {
private:
    void runChild(const el::String &mode, const int expectedExit) {
        const auto child = el::system::Subprocess::create(
            el::path::Path{el::unittest::fh::unitTestExecutablePath()},
            el::text::StringList{"name:NativeDriverChild"_el, "+name:NativeDriverChild"_el, "--no-color"_el},
            el::system::SubprocessOptions{}.setEnvironmentChanges(
                el::system::EnvironmentChanges{}.setVariable("ERBSLAND_NATIVE_DRIVER_TEST"_el, mode)));
        child->start();
        const auto status = child->wait(el::time::TimeDelta::seconds(10));
        REQUIRE(status.has_value());
        REQUIRE_EQUAL(status->exitCode(), std::optional<std::int32_t>{expectedExit});
    }

public:
    void testWakePropagatesPlatformFailure() {
        const auto calls = std::make_shared<NativeTestCalls>();
        auto driver = NativeTestDriver{calls};
        auto &interface = static_cast<el::event::EventLoopDriver &>(driver);
        calls->wakeError = EBADF;
        auto caught = false;
        try {
            interface.wake();
        } catch (const el::system::PlatformFailure &failure) {
            caught = true;
            REQUIRE(failure.context() != nullptr);
            REQUIRE_FALSE(failure.reason().isEmpty());
        }
        REQUIRE(caught);
    }

    void testWakeFailureBypassesRecoverableCallbackPolicy() {
        const auto calls = std::make_shared<NativeTestCalls>();
        const auto driver = std::make_shared<NativeTestDriver>(calls);
        const auto loop = std::make_shared<el::event::impl::EventLoop>(
            driver, el::event::EventLoopOptions{}.setErrorPolicy(el::event::EventLoopErrorPolicy::Ignore));
        loop->invoke([&]() -> void {
            calls->wakeError = EBADF;
            loop->invoke([]() -> void {});
        });
        REQUIRE_THROWS_AS(el::system::PlatformFailure, loop->runOnce());
        REQUIRE_FALSE(loop->hasError());
        REQUIRE_EQUAL(loop->queuedEventCount(), 1U);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNativeWakeAndFailureChildren() {
        runChild("wake-before"_el, 0);
        runChild("wake-during"_el, 0);
        runChild("stop-failure"_el, 73);
        runChild("quit-failure"_el, 73);
        runChild("backend-wake-failure"_el, 73);
#ifndef ERBSLAND_OS_WINDOWS
        runChild("remove-failure"_el, 73);
#endif
    }

    SKIP_BY_DEFAULT()
    void testNativeDriverChild() {
        const auto mode = el::system::EnvironmentVariables{}.get("ERBSLAND_NATIVE_DRIVER_TEST"_el);
        if (!mode.has_value()) {
            return;
        }
        std::set_terminate([]() -> void {
            const auto failure = std::current_exception();
            if (failure != nullptr) {
                try {
                    std::rethrow_exception(failure);
                } catch (const el::system::PlatformFailure &error) {
                    if (error.context() != nullptr) {
                        std::_Exit(73);
                    }
                } catch (...) {}
            }
            std::_Exit(74);
        });
        const auto calls = std::make_shared<NativeTestCalls>();
        const auto driver = std::make_shared<NativeTestDriver>(calls);
        if (*mode == "wake-before"_el) {
            driver->wake();
            driver->wait();
            driver->wake();
            driver->wait();
            return;
        }
        if (*mode == "wake-during"_el) {
            auto entered = std::atomic<bool>{false};
            auto worker = std::thread{[&]() -> void {
                entered.store(true);
                driver->wait(); // No native timeout can mask a missed wake.
            }};
            while (!entered.load()) {
                std::this_thread::yield();
            }
            driver->wake();
            worker.join();
            return;
        }
#ifndef ERBSLAND_OS_WINDOWS
        if (*mode == "remove-failure"_el) {
            int descriptors[2]{};
            REQUIRE_EQUAL(::socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors), 0);
            auto registration =
                driver->registerDescriptor(descriptors[0], true, false, [](bool, bool, bool) -> void {});
            calls->removalError = EBADF;
            registration.reset();
            std::_Exit(74);
        }
#endif
        calls->wakeError = EBADF;
        auto loop = el::event::impl::EventLoop{driver};
        if (*mode == "stop-failure"_el) {
            loop.stop();
        } else if (*mode == "quit-failure"_el) {
            loop.quit();
        } else if (*mode == "backend-wake-failure"_el) {
            loop.wakeFromBackend();
        }
        std::_Exit(74);
    }
};
