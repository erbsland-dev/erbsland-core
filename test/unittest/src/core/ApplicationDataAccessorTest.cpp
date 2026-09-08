// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/impl/application_data/ApplicationDataAccessor.hpp>
#include <erbsland/core/impl/application_data/ApplicationEventData.hpp>
#include <erbsland/core/impl/application_data/ApplicationLifecycleData.hpp>
#include <erbsland/core/impl/application_data/ApplicationPartsData.hpp>
#include <erbsland/core/impl/ApplicationDataImpl.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace application_data_accessor_test {

/// Minimal value used to exercise the generic lazy accessor.
struct TestData {
    int value{}; ///< Test value.
};

}

using namespace application_data_accessor_test;

TESTED_TARGETS(ApplicationDataAccessor ApplicationData ApplicationDataImpl ApplicationPartsData)
class ApplicationDataAccessorTest final : public el::UnitTest {
public:
    void testCreatesOneStableInstance() {
        auto accessor = el::core::impl::ApplicationDataAccessor<TestData>{};

        REQUIRE(accessor.getIfExists() == nullptr);
        const auto first = accessor.get();
        const auto second = accessor.get();

        REQUIRE(first != nullptr);
        REQUIRE_EQUAL(first, second);
        REQUIRE_EQUAL(accessor.getIfExists(), first);
    }

    void testConcurrentCreationInvokesFactoryOnce() {
        auto factoryCalls = std::atomic<std::size_t>{};
        auto accessor = el::core::impl::ApplicationDataAccessor<TestData>{[&factoryCalls]() {
            ++factoryCalls;
            std::this_thread::yield();
            return std::make_shared<TestData>();
        }};
        auto results = std::vector<std::shared_ptr<TestData>>(16);
        auto threads = std::vector<std::thread>{};
        threads.reserve(results.size());

        for (auto index = std::size_t{}; index < results.size(); ++index) {
            threads.emplace_back([&accessor, &results, index]() -> void { results[index] = accessor.get(); });
        }
        for (auto &thread : threads) {
            thread.join();
        }

        REQUIRE_EQUAL(factoryCalls.load(), 1U);
        for (const auto &result : results) {
            REQUIRE_EQUAL(result, results.front());
        }
    }

    void testFactoryFailureCanBeRetried() {
        auto factoryCalls = std::size_t{};
        auto accessor =
            el::core::impl::ApplicationDataAccessor<TestData>{[&factoryCalls]() -> std::shared_ptr<TestData> {
                ++factoryCalls;
                if (factoryCalls == 1U) {
                    throw std::runtime_error{"first creation failed"};
                }
                return std::make_shared<TestData>();
            }};

        REQUIRE_THROWS_AS(std::runtime_error, accessor.get());
        REQUIRE(accessor.getIfExists() == nullptr);
        REQUIRE(accessor.get() != nullptr);
        REQUIRE_EQUAL(factoryCalls, 2U);
    }

    void testNullFactoryResultIsRejected() {
        auto accessor =
            el::core::impl::ApplicationDataAccessor<TestData>{[]() -> std::shared_ptr<TestData> { return nullptr; }};

        REQUIRE_THROWS_AS(el::err::LogicError, accessor.get());
        REQUIRE(accessor.getIfExists() == nullptr);
    }

    void testCompositionStartsEmptyAndCreatesOnlyRequestedComponent() {
        auto data = el::core::impl::ApplicationDataImpl{};

        REQUIRE(data.runtime().getIfExists() == nullptr);
        REQUIRE(data.options().getIfExists() == nullptr);
        REQUIRE(data.lifecycle().getIfExists() == nullptr);
        REQUIRE(data.parts().getIfExists() == nullptr);
        REQUIRE(data.events().getIfExists() == nullptr);
        REQUIRE(data.terminal().getIfExists() == nullptr);
        REQUIRE(data.logging().getIfExists() == nullptr);
        REQUIRE(data.random().getIfExists() == nullptr);
        REQUIRE(data.cryptology().getIfExists() == nullptr);
        REQUIRE(data.resources().getIfExists() == nullptr);
        REQUIRE(data.system().getIfExists() == nullptr);

        REQUIRE(data.runtime().get() != nullptr);
        REQUIRE(data.runtime().getIfExists() != nullptr);
        REQUIRE(data.options().getIfExists() == nullptr);
        REQUIRE(data.lifecycle().getIfExists() == nullptr);
        REQUIRE(data.parts().getIfExists() == nullptr);
        REQUIRE(data.events().getIfExists() == nullptr);
        REQUIRE(data.terminal().getIfExists() == nullptr);
        REQUIRE(data.logging().getIfExists() == nullptr);
        REQUIRE(data.random().getIfExists() == nullptr);
        REQUIRE(data.cryptology().getIfExists() == nullptr);
        REQUIRE(data.resources().getIfExists() == nullptr);
        REQUIRE(data.system().getIfExists() == nullptr);
    }

    void testPartBindingsDoNotCreateOwnershipCycles() {
        auto weakEventData = std::weak_ptr<el::core::impl::ApplicationEventData>{};
        auto weakLifecycleData = std::weak_ptr<el::core::impl::ApplicationLifecycleData>{};
        auto weakPartsData = std::weak_ptr<el::core::impl::ApplicationPartsData>{};
        {
            auto data = std::make_shared<el::core::impl::ApplicationDataImpl>();
            const auto eventData = data->events().get();
            const auto lifecycleData = data->lifecycle().get();
            const auto partsData = data->parts().get();
            static_cast<void>(partsData->manager(eventData, lifecycleData));
            weakEventData = eventData;
            weakLifecycleData = lifecycleData;
            weakPartsData = partsData;
        }

        REQUIRE(weakEventData.expired());
        REQUIRE(weakLifecycleData.expired());
        REQUIRE(weakPartsData.expired());
    }
};
