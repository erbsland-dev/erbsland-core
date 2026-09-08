// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/impl/ApplicationDataImpl.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>

namespace erbsland::test::applicationtestscopetest {

class DerivedApplication final : public el::core::Application {
public:
    DerivedApplication() = default;

public:
    bool mainCalled{false};

protected:
    auto main() -> el::unit::ExitCode override {
        mainCalled = true;
        return el::unit::ExitCode{7};
    }
};

class HookApplicationData final : public el::core::impl::ApplicationDataImpl {
public:
    inline static std::atomic<int> optionsAccessCount{};

public:
    auto options() const noexcept -> const el::core::impl::ApplicationOptionsDataAccessor & override {
        ++optionsAccessCount;
        return ApplicationDataImpl::options();
    }
};

class LazyCleanupApplicationData final : public el::core::impl::ApplicationDataImpl {
public:
    inline static bool lifecycleWasCreated{};
    inline static bool loggingWasCreated{};
    inline static bool terminalWasCreated{};

public:
    ~LazyCleanupApplicationData() override {
        lifecycleWasCreated = ApplicationDataImpl::lifecycle().getIfExists() != nullptr;
        loggingWasCreated = ApplicationDataImpl::logging().getIfExists() != nullptr;
        terminalWasCreated = ApplicationDataImpl::terminal().getIfExists() != nullptr;
    }
};

}

using namespace erbsland::test::applicationtestscopetest;
using namespace el::text::literals;

TESTED_TARGETS(Application ApplicationData ApplicationDataImpl ApplicationTestScope)
class ApplicationTestScopeTest final : public el::UnitTest {
public:
    void testAccessOutsideScopeThrows() {
        REQUIRE_THROWS_AS(IllegalApplicationInstanceAccess, el::core::Application::instance());
    }

    void testScopedApplicationIsTheSingleton() {
        auto scope = ApplicationTestScope<>{};

        const auto applicationInstance = &el::core::Application::instance();
        const auto application = &el::core::application();
        const auto scopeApplication = &scope.app();
        REQUIRE_EQUAL(applicationInstance, scopeApplication);
        REQUIRE_EQUAL(application, scopeApplication);
    }

    void testDerivedApplicationScope() {
        auto scope = ApplicationTestScope<DerivedApplication>{};

        const auto exitCode = scope.app().run();
        REQUIRE_EQUAL(exitCode, 7);
        REQUIRE(scope.app().mainCalled);
    }

    void testCustomApplicationDataIsInstalled() {
        HookApplicationData::optionsAccessCount = 0;
        auto scope = ApplicationTestScope<el::core::Application, HookApplicationData>{};

        REQUIRE(scope.app().options());
        REQUIRE_GREATER(HookApplicationData::optionsAccessCount, 0);
    }

    void testNoLocalApplicationInstanceTransfersTemporaryDataToLocalInstance() {
        auto scope = ApplicationTestScope<>{ApplicationTestScope<>::NoLocalAppInstance{}};

        auto &temporaryApplication = el::core::Application::instance();
        temporaryApplication.info().setApplicationName("Temporary"_el);
        auto application = el::core::Application{};

        const auto applicationInstance = &el::core::Application::instance();
        REQUIRE_EQUAL(applicationInstance, &application);
        const auto applicationName = application.info().applicationName();
        REQUIRE_EQUAL(applicationName, "Temporary"_el);
        REQUIRE_THROWS_AS(IllegalApplicationInstanceAccess, scope.app());
    }

    void testCleanupDoesNotCreateUnusedComponents() {
        LazyCleanupApplicationData::lifecycleWasCreated = true;
        LazyCleanupApplicationData::loggingWasCreated = true;
        LazyCleanupApplicationData::terminalWasCreated = true;
        { auto scope = ApplicationTestScope<el::core::Application, LazyCleanupApplicationData>{}; }

        REQUIRE_FALSE(LazyCleanupApplicationData::lifecycleWasCreated);
        REQUIRE_FALSE(LazyCleanupApplicationData::loggingWasCreated);
        REQUIRE_FALSE(LazyCleanupApplicationData::terminalWasCreated);
    }

    void testRunDoesNotCreateUnusedLifecycleData() {
        LazyCleanupApplicationData::lifecycleWasCreated = true;
        {
            auto scope = ApplicationTestScope<el::core::Application, LazyCleanupApplicationData>{};
            scope.app().setMainFn([]() -> el::unit::ExitCode { return el::unit::ExitCode::success(); });
            REQUIRE_EQUAL(scope.app().run(), 0);
        }

        REQUIRE_FALSE(LazyCleanupApplicationData::lifecycleWasCreated);
    }

    void testQuitDoesNotCreateUnusedLifecycleData() {
        LazyCleanupApplicationData::lifecycleWasCreated = true;
        {
            auto scope = ApplicationTestScope<el::core::Application, LazyCleanupApplicationData>{};
            scope.app().quit();
        }

        REQUIRE_FALSE(LazyCleanupApplicationData::lifecycleWasCreated);
    }

    void testPartManagerDoesNotCreateUnusedLifecycleData() {
        LazyCleanupApplicationData::lifecycleWasCreated = true;
        {
            auto scope = ApplicationTestScope<el::core::Application, LazyCleanupApplicationData>{};
            REQUIRE(scope.app().partManager() != nullptr);
        }

        REQUIRE_FALSE(LazyCleanupApplicationData::lifecycleWasCreated);
    }
};
