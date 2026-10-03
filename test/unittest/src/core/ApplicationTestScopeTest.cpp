// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/core/impl/application_data/ApplicationRandomData.hpp>
#include <erbsland/core/impl/application_data/ApplicationResourceData.hpp>
#include <erbsland/core/impl/ApplicationDataAccess.hpp>
#include <erbsland/core/impl/ApplicationDataImpl.hpp>
#include <erbsland/core/impl/ApplicationExecutionContext.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <future>
#include <thread>

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
        REQUIRE_THROWS_AS(IllegalApplicationInstanceAccess, el::core::impl::applicationData());
    }

    void testDataOnlyAccessIsAdoptedByTheApplication() {
        auto scope = ApplicationTestScope<>{ApplicationTestScope<>::NoLocalAppInstance{}};
        const auto data = el::core::impl::applicationData();
        const auto randomData = data->random().get();
        REQUIRE(data->system().getIfExists() == nullptr);
        REQUIRE(data->logging().getIfExists() == nullptr);
        auto application = el::core::Application{};
        REQUIRE(el::core::impl::applicationData() == data);
        REQUIRE(&application.secureRandom() == &randomData->secureRandom());
        REQUIRE(&el::core::application() == &application);
    }

    void testComponentRetentionDoesNotRetainApplicationData() {
        auto randomData = std::shared_ptr<el::core::impl::ApplicationRandomData>{};
        auto weakData = std::weak_ptr<el::core::impl::ApplicationData>{};
        auto weakResources = std::weak_ptr<el::core::impl::ApplicationResourceData>{};
        {
            const auto scope = ApplicationTestScope<>{};
            const auto data = el::core::impl::applicationData();
            weakData = data;
            weakResources = data->resources().get();
            randomData = data->random().get();
        }
        REQUIRE(weakData.expired());
        REQUIRE(weakResources.expired());
        REQUIRE_EQUAL(
            randomData->secureRandom().buildByteBlock(el::unit::ByteCount{16U}).length(), el::unit::ByteCount{16U});
        const auto weakRandom = std::weak_ptr<el::core::impl::ApplicationRandomData>{randomData};
        randomData.reset();
        REQUIRE(weakRandom.expired());
    }

    void testUnregisteredGenerationIsNotReused() {
        auto scope = ApplicationTestScope<>{ApplicationTestScope<>::NoLocalAppInstance{}};
        auto firstData = el::core::impl::ApplicationDataPtr{};
        {
            auto application = el::core::Application{};
            firstData = el::core::impl::applicationData();
            {
                auto duplicate = el::core::Application{};
                REQUIRE(el::core::impl::applicationData() == firstData);
                REQUIRE(&el::core::application() == &application);
            }
            REQUIRE(el::core::impl::applicationData() == firstData);
        }
        const auto nextData = el::core::impl::applicationData();
        REQUIRE(nextData != firstData);
        {
            const auto executionScope = el::core::impl::ApplicationExecutionContext{firstData};
            REQUIRE(el::core::impl::applicationData() == firstData);
            auto nextApplication = el::core::Application{};
            REQUIRE(&nextApplication.secureRandom() == &nextData->random().get()->secureRandom());
        }
    }

    void testLinkedManagerRetainsDataWithoutAnApplicationFacade() {
        auto scope = ApplicationTestScope<>{};
        auto manager = el::core::impl::ApplicationInstanceManager{};
        auto previousData = manager.retainData();
        const auto weakPrevious = std::weak_ptr<el::core::impl::ApplicationData>{previousData};
        previousData.reset();
        manager.linkWith(scope.app());
        REQUIRE(weakPrevious.expired());
        REQUIRE(manager.retainData() == el::core::impl::applicationData());
        REQUIRE(&manager.application().secureRandom() == &scope.app().secureRandom());
        manager.linkWith(scope.app());
        REQUIRE(manager.retainData() == el::core::impl::applicationData());
    }

    void testExplicitScopeWorksWithoutGlobalApplicationAccess() {
        const auto data = std::make_shared<el::core::impl::ApplicationDataImpl>();
        {
            const auto executionScope = el::core::impl::ApplicationExecutionContext{data};
            REQUIRE(el::core::impl::applicationData() == data);
            REQUIRE_THROWS_AS(IllegalApplicationInstanceAccess, el::core::application());
        }
        REQUIRE_THROWS_AS(IllegalApplicationInstanceAccess, el::core::impl::applicationData());
    }

    void testExecutionScopesAreThreadLocalAndRestoreOnWorkerReuse() {
        const auto scope = ApplicationTestScope<>{};
        const auto processData = el::core::impl::applicationData();
        const auto firstData = std::make_shared<el::core::impl::ApplicationDataImpl>();
        const auto secondData = std::make_shared<el::core::impl::ApplicationDataImpl>();
        auto entered = std::promise<void>{};
        auto entry = entered.get_future();
        auto release = std::promise<void>{};
        const auto gate = release.get_future();
        auto valid = false;
        auto worker = std::jthread{[&]() -> void {
            {
                const auto executionScope = el::core::impl::ApplicationExecutionContext{firstData};
                entered.set_value();
                gate.wait();
                valid = el::core::impl::applicationData() == firstData;
            }
            valid = valid && el::core::impl::applicationData() == processData;
            {
                const auto executionScope = el::core::impl::ApplicationExecutionContext{secondData};
                valid = valid && el::core::impl::applicationData() == secondData;
            }
            valid = valid && el::core::impl::ApplicationExecutionContext::current() == nullptr;
        }};
        entry.wait();
        const auto mainData = el::core::impl::applicationData();
        release.set_value();
        worker.join();
        REQUIRE(mainData == processData);
        REQUIRE(valid);
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
