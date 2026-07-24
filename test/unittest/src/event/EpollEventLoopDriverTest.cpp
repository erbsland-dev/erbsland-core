// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/impl/EpollEventLoopDriver.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <unistd.h>

#include <array>

using namespace el::time;

TESTED_TARGETS(EpollEventLoopDriver)
class EpollEventLoopDriverTest final : public el::UnitTest {
public:
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
};
