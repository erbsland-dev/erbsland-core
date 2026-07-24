// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/impl/KqueueEventLoopDriver.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(KqueueEventLoopDriver)
class KqueueEventLoopDriverTest final : public el::UnitTest {
public:
    void testConstruction() {
        const auto driver = el::event::impl::KqueueEventLoopDriver{};
        static_cast<void>(driver);
    }
};
