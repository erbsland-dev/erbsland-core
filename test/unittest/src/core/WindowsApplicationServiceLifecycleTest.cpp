// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/unit/ExitCode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Application)
class WindowsApplicationServiceLifecycleTest final : public el::UnitTest {
public:
    void testConsoleLaunchFallsBackToInteractiveLifecycle() {
        auto scope = ApplicationTestScope<el::core::Application>{};
        auto &application = scope.app();
        application.enableServiceLifecycle();
        application.setMainFn([]() -> el::unit::ExitCode { return el::unit::ExitCode{23}; });

        REQUIRE_EQUAL(application.run(), 23);
    }
};
