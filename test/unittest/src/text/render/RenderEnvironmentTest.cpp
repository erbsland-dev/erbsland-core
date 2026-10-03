// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include "../../core/ApplicationTestScope.hpp"

#include <erbsland/Diagnostic.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/Loader.hpp>
#include <erbsland/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Delimiters Environment EnvironmentOptions LayoutSource Loader RenderError RenderErrorContext)
class RenderEnvironmentTest final : public el::UnitTest {
private:
    static constexpr auto cBadLayout = "bad"_el;
    static constexpr auto cEmptyLayout = "empty"_el;
    static constexpr auto cLateKey = "late"_el;
    static constexpr auto cNameKey = "name"_el;
    static constexpr auto cPageLayout = "page"_el;
    static constexpr auto cProfileKey = "profile"_el;
    static constexpr auto cUserKey = "user"_el;
    static constexpr auto cValueKey = "value"_el;

public:
    void testLineStatementsCommentsAndExpressions() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("loop"_el, "before\n  //% for value in [1, 2]\n[[ value ]]\n\t//% endfor\nafter"_el);
        loader->set(
            "mixed"_el,
            "//% set value = \"%}\"\r\n[[ value ]]\n{% set other = 7 %}[[ other ]]\ninline //% set value = 9\n//% set final = 1"_el);
        loader->set("expression"_el, "  //= \"hello\"\n// comment\n  //# removed\nend"_el);
        loader->set(cEmptyLayout, "//# comment at end"_el);
        loader->set("empty_comment"_el, "{# comment at end #}"_el);
        loader->set(cBadLayout, "//% if true\nbody\n"_el);
        auto options = EnvironmentOptions{};
        options.setExpressionDelimiters(Delimiters{"[["_el, "]]"_el, "//="_el});
        options.setStatementDelimiters(Delimiters{"{%"_el, "%}"_el, "//%"_el});
        options.setCommentDelimiters(Delimiters{"{#"_el, "#}"_el, "//#"_el});
        const auto environment = Environment::create(options);
        environment->addLayoutLoader(loader);
        REQUIRE_EQUAL(environment->render("loop"_el), "before\n1\n2\nafter"_el);
        REQUIRE_EQUAL(environment->render("mixed"_el), "%}\n7\ninline //% set value = 9\n"_el);
        REQUIRE_EQUAL(environment->render("expression"_el), "hello// comment\nend"_el);
        REQUIRE_EQUAL(environment->render(cEmptyLayout), ""_el);
        REQUIRE_EQUAL(environment->render("empty_comment"_el), ""_el);
        REQUIRE_THROWS_AS(RenderError, environment->render(cBadLayout));
    }

    void testAmbiguousLinePrefixesAreRejected() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = EnvironmentOptions{};
        options.setStatementDelimiters(Delimiters{"{%"_el, "%}"_el, "//"_el});
        options.setCommentDelimiters(Delimiters{"{#"_el, "#}"_el, "//#"_el});
        REQUIRE_THROWS(Environment::create(options));
    }

    void testStaticCommentsAndUnicode() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cEmptyLayout, ""_el);
        loader->set(cPageLayout, "Hello {# removed #}wörld!"_el);
        const auto environment = environmentFor(loader);

        REQUIRE_EQUAL(environment->render(cEmptyLayout), ""_el);
        REQUIRE_EQUAL(environment->render(cPageLayout), "Hello wörld!"_el);
    }

    void testExpressionsAndContextPrecedence() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, "{{ name }}|{{ user.profile.name }}|{{ missing.path }}|{{ late }}"_el);
        const auto environment = environmentFor(loader);

        auto globalProfile = ValueMap{};
        globalProfile.set(cNameKey, "global nested"_el);
        auto globalUser = ValueMap{};
        globalUser.set(cProfileKey, globalProfile);
        environment->setGlobalContext(
            Context{}.set(cNameKey, "global"_el).set(cUserKey, globalUser).set(cLateKey, "global late"_el));

        auto localProfile = ValueMap{};
        localProfile.set(cNameKey, "local nested"_el);
        auto localUser = ValueMap{};
        localUser.set(cProfileKey, localProfile);
        auto local = Context{};
        local.set(cNameKey, "local"_el).set(cUserKey, localUser).set(cLateKey, ValueCallbackFn{[]() -> Value {
            return "callback"_el;
        }});

        REQUIRE_EQUAL(environment->render(cPageLayout, local), "local|local nested||callback"_el);
    }

    void testWhitespaceMarkers() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("none"_el, "A \n{{ value }} \nB"_el);
        loader->set("left"_el, "A \n{{- value }} \nB"_el);
        loader->set("right"_el, "A \n{{ value -}} \nB"_el);
        loader->set("both"_el, "A \n{{- value -}} \nB"_el);
        loader->set("comment"_el, "A \n{#- hidden -#} \nB"_el);
        const auto environment = environmentFor(loader);
        const auto context = Context{}.set(cValueKey, "X"_el);

        REQUIRE_EQUAL(environment->render("none"_el, context), "A \nX \nB"_el);
        REQUIRE_EQUAL(environment->render("left"_el, context), "AX \nB"_el);
        REQUIRE_EQUAL(environment->render("right"_el, context), "A \nXB"_el);
        REQUIRE_EQUAL(environment->render("both"_el, context), "AXB"_el);
        REQUIRE_EQUAL(environment->render("comment"_el), "AB"_el);
    }

    void testCustomDelimitersAndSyntaxErrors() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("custom"_el, "Hello [[ name ]]"_el);
        loader->set(cBadLayout, "before [[ name"_el);
        auto options = EnvironmentOptions{};
        options.setExpressionDelimiters(Delimiters{"[["_el, "]]"_el});
        const auto environment = Environment::create(options);
        environment->addLayoutLoader(loader);

        REQUIRE_EQUAL(environment->render("custom"_el, Context{}.set(cNameKey, "Ada"_el)), "Hello Ada"_el);
        REQUIRE_THROWS_AS(RenderError, environment->render(cBadLayout));

        try {
            static_cast<void>(environment->render(cBadLayout));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Syntax);
            REQUIRE_EQUAL(error.context().layout(), cBadLayout);
            REQUIRE_EQUAL(error.context().origin(), "memory:bad"_el);
            REQUIRE_FALSE(error.context().location().isUndefined());
            REQUIRE_EQUAL(error.diagnostic()->sourceName(), cBadLayout);
            REQUIRE_EQUAL(error.diagnostic()->sourcePath(), "memory:bad"_el);
        }
    }

    void testValidationAndStrictRendering() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("container"_el, "{{ value }}"_el);
        const auto environment = environmentFor(loader);

        REQUIRE_THROWS_AS(RenderError, environment->render("../page"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("UPPER"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("missing"_el));

        try {
            static_cast<void>(environment->render("container"_el, Context{}.set(cValueKey, ValueMap{})));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().layout(), "container"_el);
        }
    }

    void testLoaderPriorityAndCacheReuse() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto first = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        const auto second = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        first->set(cPageLayout, "first"_el);
        second->set(cPageLayout, "second"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(first, 0);
        environment->addLayoutLoader(second, 10);

        REQUIRE_EQUAL(environment->render(cPageLayout), "second"_el);
        REQUIRE_EQUAL(environment->render(cPageLayout), "second"_el);
        REQUIRE_EQUAL(first->loadCount(), std::size_t{0U});
        REQUIRE_EQUAL(second->loadCount(), std::size_t{1U});

        const auto equalPriority = Environment::create();
        equalPriority->addLayoutLoader(first);
        equalPriority->addLayoutLoader(second);
        REQUIRE_EQUAL(equalPriority->render(cPageLayout), "first"_el);
    }

    void testReloadAndFailedReloadRecovery() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, "old"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();

        REQUIRE_EQUAL(environment->render(cPageLayout), "old"_el);
        loader->set(cPageLayout, "new {{ value }}"_el);
        REQUIRE_EQUAL(environment->render(cPageLayout, Context{}.set(cValueKey, "generation"_el)), "new generation"_el);

        loader->set(cPageLayout, "broken {{"_el);
        REQUIRE_THROWS_AS(RenderError, environment->render(cPageLayout));
        loader->set(cPageLayout, "new {{ value }}"_el);
        REQUIRE_EQUAL(environment->render(cPageLayout, Context{}.set(cValueKey, "generation"_el)), "new generation"_el);
    }

    void testCallbackDepthLimit() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, "{{ value }}"_el);
        const auto environment = environmentFor(loader);
        const auto callback = std::make_shared<ValueCallbackFn>();
        *callback = [callback]() -> Value { return Value{*callback}; };

        try {
            static_cast<void>(environment->render(cPageLayout, Context{}.set(cValueKey, Value{*callback})));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Limit);
        }
    }

    void testConcurrentRenderSnapshots() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, "old"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();
        REQUIRE_EQUAL(environment->render(cPageLayout), "old"_el);

        auto failed = std::atomic_bool{false};
        auto workers = std::vector<std::thread>{};
        for (auto worker = 0; worker < 4; ++worker) {
            workers.emplace_back([&]() -> void {
                for (auto iteration = 0; iteration < 25; ++iteration) {
                    try {
                        const auto result = environment->render(cPageLayout);
                        if (result != "old"_el && result != "new"_el) {
                            failed = true;
                        }
                    } catch (...) {
                        failed = true;
                    }
                }
            });
        }
        loader->set(cPageLayout, "new"_el);
        for (auto &worker : workers) {
            worker.join();
        }
        REQUIRE_FALSE(failed.load());
        REQUIRE_EQUAL(environment->render(cPageLayout), "new"_el);
    }

private:
    [[nodiscard]] static auto environmentFor(const LoaderPtr &loader) -> EnvironmentPtr {
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment;
    }
};
