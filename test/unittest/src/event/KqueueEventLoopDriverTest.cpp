// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/impl/KqueueEventLoopDriver.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/system/impl/PosixErrorContext.hpp>
#include <erbsland/unittest/UnitTest.hpp>

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
};
