// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include "../../core/ApplicationTestScope.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/Loader.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <barrier>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Environment)
class RenderConcurrencyTest final : public el::UnitTest {
private:
    static constexpr auto cFirstLayout = "first"_el;
    static constexpr auto cPageLayout = "page"_el;
    static constexpr auto cSecondLayout = "second"_el;
    static constexpr auto cValueKey = "value"_el;

    /// A loader callback for coordinating loads and exercising environment reentry.
    class CallbackLoader final : public Loader {
    public:
        /// Create a loader forwarding validated names to the callback.
        /// @param callback The source lookup operation.
        explicit CallbackLoader(std::function<std::optional<LayoutSource>(const el::String &)> callback) :
            _callback{std::move(callback)} {}

    public: // implement Loader
        [[nodiscard]] auto load(const el::String &layout) -> std::optional<LayoutSource> override {
            return _callback(layout);
        }

    private:
        std::function<std::optional<LayoutSource>(const el::String &)> _callback; ///< Source lookup operation.
    };

public:
    void testConcurrentFirstRenderCompilesOnce() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, "A{% include 'part' %}C"_el);
        loader->set("part"_el, "B"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        auto start = std::barrier{9};
        auto renders = std::vector<std::future<el::String>>{};
        for (auto worker = 0; worker < 8; ++worker) {
            renders.push_back(std::async(std::launch::async, [&]() -> el::String {
                start.arrive_and_wait();
                return environment->render(cPageLayout);
            }));
        }
        start.arrive_and_wait();
        for (auto &render : renders) {
            REQUIRE_EQUAL(render.get(), "ABC"_el);
        }
        REQUIRE_EQUAL(loader->loadCount(), std::size_t{2});
    }

    void testConcurrentCachedRendersAndContextReplacement() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, "{{ first }}{{ second }}"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        const auto first = Context{}.set(cFirstLayout, "A"_el).set(cSecondLayout, "A"_el);
        const auto second = Context{}.set(cFirstLayout, "B"_el).set(cSecondLayout, "B"_el);
        environment->setGlobalContext(first);
        REQUIRE_EQUAL(environment->render(cPageLayout), "AA"_el);
        auto start = std::barrier{9};
        auto renders = std::vector<std::future<bool>>{};
        for (auto worker = 0; worker < 8; ++worker) {
            renders.push_back(std::async(std::launch::async, [&]() -> bool {
                start.arrive_and_wait();
                for (auto iteration = 0; iteration < 64; ++iteration) {
                    const auto result = environment->render(cPageLayout);
                    if (result != "AA"_el && result != "BB"_el) {
                        return false;
                    }
                }
                return true;
            }));
        }
        start.arrive_and_wait();
        for (auto iteration = 0; iteration < 128; ++iteration) {
            environment->setGlobalContext(first);
            environment->setGlobalContext(second);
        }
        for (auto &render : renders) {
            REQUIRE(render.get());
        }
        REQUIRE_EQUAL(environment->render(cPageLayout), "BB"_el);
        REQUIRE_EQUAL(loader->loadCount(), std::size_t{1});
    }

    void testCachedRendersContinueWhileAnotherLayoutLoads() {
        auto applicationScope = ApplicationTestScope<>{};
        auto entered = std::promise<void>{};
        auto enteredFuture = entered.get_future();
        auto release = std::promise<void>{};
        auto releaseFuture = release.get_future();
        const auto loader =
            std::make_shared<CallbackLoader>([&](const el::String &layout) -> std::optional<LayoutSource> {
                if (layout == "slow"_el) {
                    entered.set_value();
                    releaseFuture.wait();
                }
                return LayoutSource{"{{ value }}"_el, layout, "1"_el};
            });
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->setGlobalContext(Context{}.set(cValueKey, "old"_el));
        REQUIRE_EQUAL(environment->render("cached"_el), "old"_el);
        auto slow = std::async(std::launch::async, [&]() -> el::String { return environment->render("slow"_el); });
        const auto enteredStatus = enteredFuture.wait_for(std::chrono::seconds{2});
        auto cached = std::async(std::launch::async, [&]() -> el::String {
            environment->setGlobalContext(Context{}.set(cValueKey, "new"_el));
            return environment->render("cached"_el);
        });
        const auto cachedStatus = cached.wait_for(std::chrono::seconds{2});
        // Release the loader before checking results, so a regression cannot strand a worker on the gate.
        release.set_value();
        const auto cachedResult = cached.get();
        const auto slowResult = slow.get();
        REQUIRE(enteredStatus == std::future_status::ready);
        REQUIRE(cachedStatus == std::future_status::ready);
        REQUIRE_EQUAL(cachedResult, "new"_el);
        REQUIRE_EQUAL(slowResult, "new"_el);
    }

    void testLoaderCanReenterEnvironment() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto environment = Environment::create();
        const auto loader =
            std::make_shared<CallbackLoader>([&](const el::String &layout) -> std::optional<LayoutSource> {
                if (layout == "outer"_el) {
                    environment->setGlobalContext(Context{}.set(cValueKey, environment->render("inner"_el)));
                    return LayoutSource{"{{ value }}"_el, layout, "1"_el};
                }
                return LayoutSource{"nested"_el, layout, "1"_el};
            });
        environment->addLayoutLoader(loader);
        REQUIRE_EQUAL(environment->render("outer"_el), "nested"_el);
        REQUIRE_EQUAL(environment->render("outer"_el), "nested"_el);
    }

    void testRenderRetainsContextAcrossReentrantCallback() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, "{{ value }}{{ update }}{{ value }}"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->setGlobalContext(Context{}.set(cValueKey, "old"_el));
        auto local = Context{}.set("update"_el, ValueCallbackFn{[&]() -> Value {
            environment->setGlobalContext(Context{}.set(cValueKey, "new"_el));
            return "|"_el;
        }});
        REQUIRE_EQUAL(environment->render(cPageLayout, local), "old|old"_el);
        REQUIRE_EQUAL(environment->render(cPageLayout, local), "new|new"_el);
    }
};
