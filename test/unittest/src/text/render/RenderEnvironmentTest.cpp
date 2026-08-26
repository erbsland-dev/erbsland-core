// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/err/Diagnostic.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/Loader.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Delimiters Environment EnvironmentOptions LayoutSource Loader RenderError RenderErrorContext)
class RenderEnvironmentTest final : public el::UnitTest {
public:
    void testStaticCommentsAndUnicode() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("empty"_el, ""_el);
        loader->set("page"_el, "Hello {# removed #}wörld!"_el);
        const auto environment = environmentFor(loader);

        REQUIRE_EQUAL(environment->render("empty"_el), ""_el);
        REQUIRE_EQUAL(environment->render("page"_el), "Hello wörld!"_el);
    }

    void testExpressionsAndContextPrecedence() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "{{ name }}|{{ user.profile.name }}|{{ missing.path }}|{{ late }}"_el);
        const auto environment = environmentFor(loader);

        auto globalProfile = ValueMap{};
        globalProfile.set("name"_el, "global nested"_el);
        auto globalUser = ValueMap{};
        globalUser.set("profile"_el, globalProfile);
        environment->setGlobalContext(
            Context{}.set("name"_el, "global"_el).set("user"_el, globalUser).set("late"_el, "global late"_el));

        auto localProfile = ValueMap{};
        localProfile.set("name"_el, "local nested"_el);
        auto localUser = ValueMap{};
        localUser.set("profile"_el, localProfile);
        auto local = Context{};
        local.set("name"_el, "local"_el).set("user"_el, localUser).set("late"_el, ValueCallbackFn{[]() -> Value {
            return "callback"_el;
        }});

        REQUIRE_EQUAL(environment->render("page"_el, local), "local|local nested||callback"_el);
    }

    void testWhitespaceMarkers() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("none"_el, "A \n{{ value }} \nB"_el);
        loader->set("left"_el, "A \n{{- value }} \nB"_el);
        loader->set("right"_el, "A \n{{ value -}} \nB"_el);
        loader->set("both"_el, "A \n{{- value -}} \nB"_el);
        loader->set("comment"_el, "A \n{#- hidden -#} \nB"_el);
        const auto environment = environmentFor(loader);
        const auto context = Context{}.set("value"_el, "X"_el);

        REQUIRE_EQUAL(environment->render("none"_el, context), "A \nX \nB"_el);
        REQUIRE_EQUAL(environment->render("left"_el, context), "AX \nB"_el);
        REQUIRE_EQUAL(environment->render("right"_el, context), "A \nXB"_el);
        REQUIRE_EQUAL(environment->render("both"_el, context), "AXB"_el);
        REQUIRE_EQUAL(environment->render("comment"_el), "AB"_el);
    }

    void testCustomDelimitersAndSyntaxErrors() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("custom"_el, "Hello [[ name ]]"_el);
        loader->set("bad"_el, "before [[ name"_el);
        auto options = EnvironmentOptions{};
        options.setExpressionDelimiters(Delimiters{"[["_el, "]]"_el});
        const auto environment = Environment::create(options);
        environment->addLayoutLoader(loader);

        REQUIRE_EQUAL(environment->render("custom"_el, Context{}.set("name"_el, "Ada"_el)), "Hello Ada"_el);
        REQUIRE_THROWS_AS(RenderError, environment->render("bad"_el));

        try {
            static_cast<void>(environment->render("bad"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Syntax);
            REQUIRE_EQUAL(error.context().layout(), "bad"_el);
            REQUIRE_EQUAL(error.context().origin(), "memory:bad"_el);
            REQUIRE_FALSE(error.context().location().isUndefined());
            REQUIRE_EQUAL(error.diagnostic()->sourceName(), "bad"_el);
            REQUIRE_EQUAL(error.diagnostic()->sourcePath(), "memory:bad"_el);
        }
    }

    void testValidationAndStrictRendering() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("container"_el, "{{ value }}"_el);
        const auto environment = environmentFor(loader);

        REQUIRE_THROWS_AS(RenderError, environment->render("../page"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("UPPER"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("missing"_el));

        try {
            static_cast<void>(environment->render("container"_el, Context{}.set("value"_el, ValueMap{})));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().layout(), "container"_el);
        }
    }

    void testLoaderPriorityAndCacheReuse() {
        const auto first = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        const auto second = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        first->set("page"_el, "first"_el);
        second->set("page"_el, "second"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(first, 0);
        environment->addLayoutLoader(second, 10);

        REQUIRE_EQUAL(environment->render("page"_el), "second"_el);
        REQUIRE_EQUAL(environment->render("page"_el), "second"_el);
        REQUIRE_EQUAL(first->loadCount(), std::size_t{0U});
        REQUIRE_EQUAL(second->loadCount(), std::size_t{1U});

        const auto equalPriority = Environment::create();
        equalPriority->addLayoutLoader(first);
        equalPriority->addLayoutLoader(second);
        REQUIRE_EQUAL(equalPriority->render("page"_el), "first"_el);
    }

    void testReloadAndFailedReloadRecovery() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "old"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();

        REQUIRE_EQUAL(environment->render("page"_el), "old"_el);
        loader->set("page"_el, "new {{ value }}"_el);
        REQUIRE_EQUAL(environment->render("page"_el, Context{}.set("value"_el, "generation"_el)), "new generation"_el);

        loader->set("page"_el, "broken {{"_el);
        REQUIRE_THROWS_AS(RenderError, environment->render("page"_el));
        loader->set("page"_el, "new {{ value }}"_el);
        REQUIRE_EQUAL(environment->render("page"_el, Context{}.set("value"_el, "generation"_el)), "new generation"_el);
    }

    void testCallbackDepthLimit() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "{{ value }}"_el);
        const auto environment = environmentFor(loader);
        const auto callback = std::make_shared<ValueCallbackFn>();
        *callback = [callback]() -> Value { return Value{*callback}; };

        try {
            static_cast<void>(environment->render("page"_el, Context{}.set("value"_el, Value{*callback})));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Limit);
        }
    }

    void testConcurrentRenderSnapshots() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "old"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();
        REQUIRE_EQUAL(environment->render("page"_el), "old"_el);

        auto failed = std::atomic_bool{false};
        auto workers = std::vector<std::thread>{};
        for (auto worker = 0; worker < 4; ++worker) {
            workers.emplace_back([&]() -> void {
                for (auto iteration = 0; iteration < 25; ++iteration) {
                    try {
                        const auto result = environment->render("page"_el);
                        if (result != "old"_el && result != "new"_el) {
                            failed = true;
                        }
                    } catch (...) {
                        failed = true;
                    }
                }
            });
        }
        loader->set("page"_el, "new"_el);
        for (auto &worker : workers) {
            worker.join();
        }
        REQUIRE_FALSE(failed.load());
        REQUIRE_EQUAL(environment->render("page"_el), "new"_el);
    }

private:
    [[nodiscard]] static auto environmentFor(const LoaderPtr &loader) -> EnvironmentPtr {
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment;
    }
};
