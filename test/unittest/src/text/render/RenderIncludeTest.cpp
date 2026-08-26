// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/ToString.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(CompiledInclude Compiler Engine Environment)
class RenderIncludeTest final : public el::UnitTest {
public:
    void testBasicNestedAndIgnoredIncludes() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "A{% include \"middle\" %}D"_el);
        loader->set("middle"_el, "B{% include \"leaf\" with context %}C"_el);
        loader->set("leaf"_el, "{{ value }}"_el);
        loader->set("conditional"_el, "A{% if enabled %}{% include \"leaf\" %}{% endif %}B"_el);
        loader->set(
            "optional"_el,
            "A{% include \"missing\" ignore missing with context %}"
            "B{% include \"missing\" without context ignore missing %}C"_el);
        const auto environment = environmentFor(loader);

        REQUIRE_EQUAL(environment->render("page"_el, Context{}.set("value"_el, "X"_el)), "ABXCD"_el);
        REQUIRE_EQUAL(
            environment->render("conditional"_el, Context{}.set("enabled"_el, false).set("value"_el, "X"_el)), "AB"_el);
        REQUIRE_EQUAL(
            environment->render("conditional"_el, Context{}.set("enabled"_el, true).set("value"_el, "X"_el)), "AXB"_el);
        REQUIRE_EQUAL(environment->render("optional"_el), "ABC"_el);
    }

    void testContextModesAndPrivateAssignments() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(
            "page"_el,
            "{% set assigned = \"parent\" %}"
            "{% for item in items %}[{% include \"with\" %}][{% include \"without\" without context %}]"
            "{% endfor %}|{{ assigned }}"_el);
        loader->set(
            "with"_el,
            "{{ global }}/{{ local }}/{{ assigned }}/{{ item }}"
            "{% set assigned = \"child\" %}/{% include \"nested\" %}"_el);
        loader->set("nested"_el, "{{ assigned }}"_el);
        loader->set("without"_el, "{{ global }}/{{ local }}/{{ assigned }}/{{ item }}"_el);
        const auto environment = environmentFor(loader);
        environment->setGlobalContext(Context{}.set("global"_el, "G"_el));
        const auto local = Context{}.set("local"_el, "L"_el).set("items"_el, ValueList{"one"_el});

        REQUIRE_EQUAL(environment->render("page"_el, local), "[G/L/child/one/child][G///]|parent"_el);
    }

    void testStrictGrammarAndFailures() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("dynamic"_el, "{% include name %}"_el);
        loader->set("duplicate"_el, "{% include \"x\" with context without context %}"_el);
        loader->set("broken-ignore"_el, "{% include \"x\" ignore %}"_el);
        loader->set("duplicate-ignore"_el, "{% include \"x\" ignore missing ignore missing %}"_el);
        loader->set("incomplete-context"_el, "{% include \"x\" without value %}"_el);
        loader->set("extra"_el, "{% include \"x\" with context extra %}"_el);
        loader->set("invalid-ignored-name"_el, "{% include \"../x\" ignore missing %}"_el);
        loader->set("required"_el, "{% include \"missing\" %}"_el);
        loader->set(
            "optional-then-required"_el, "{% include \"missing\" ignore missing %}{% include \"missing\" %}"_el);
        loader->set("ignored-syntax"_el, "{% include \"broken-child\" ignore missing %}"_el);
        loader->set("broken-child"_el, "{% if %}"_el);
        const auto environment = environmentFor(loader);

        REQUIRE_THROWS_AS(RenderError, environment->render("dynamic"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("duplicate"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("broken-ignore"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("duplicate-ignore"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("incomplete-context"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("extra"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("invalid-ignored-name"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("required"_el));
        REQUIRE_THROWS_AS(RenderError, environment->render("ignored-syntax"_el));
        try {
            static_cast<void>(environment->render("optional-then-required"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::LayoutNotFound);
        }
    }

    void testCycles() {
        const auto cycleLoader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        cycleLoader->set("a"_el, "{% include \"b\" %}"_el);
        cycleLoader->set("b"_el, "{% include \"a\" %}"_el);
        try {
            static_cast<void>(environmentFor(cycleLoader)->render("a"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Syntax);
        }
    }

    void testMaximumDepth() {
        const auto depthLoader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        addDepthChain(depthLoader, 32U);
        REQUIRE_EQUAL(environmentFor(depthLoader)->render("depth/0"_el), "end"_el);
    }

    void testDepthLimit() {
        const auto depthLoader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        addDepthChain(depthLoader, 33U);
        try {
            static_cast<void>(environmentFor(depthLoader)->render("depth/0"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Limit);
        }
    }

    void testNestedDiagnosticFrames() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("root"_el, "before\n{% include \"middle\" %}"_el);
        loader->set("middle"_el, "{% include \"leaf\" %}"_el);
        loader->set("leaf"_el, "{{ value }}"_el);
        const auto environment = environmentFor(loader);
        try {
            static_cast<void>(environment->render("root"_el, Context{}.set("value"_el, ValueList{})));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().layout(), "leaf"_el);
            REQUIRE_EQUAL(error.context().origin(), "memory:leaf"_el);
            REQUIRE_EQUAL(error.context().frames().count(), el::unit::ItemCount{2U});
            REQUIRE_EQUAL(error.context().frames().get(el::unit::ItemIndex{0U}), "root:2:4"_el);
            REQUIRE_EQUAL(error.context().frames().get(el::unit::ItemIndex{1U}), "middle:1:4"_el);
        }
    }

    void testTransitiveAndOptionalReload() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "A{% include \"middle\" %}Z"_el);
        loader->set("middle"_el, "B{% include \"leaf\" %}Y"_el);
        loader->set("leaf"_el, "old"_el);
        loader->set("optional-page"_el, "A{% include \"optional\" ignore missing %}B"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();

        REQUIRE_EQUAL(environment->render("page"_el), "ABoldYZ"_el);
        loader->set("leaf"_el, "new"_el);
        REQUIRE_EQUAL(environment->render("page"_el), "ABnewYZ"_el);
        REQUIRE_EQUAL(environment->render("optional-page"_el), "AB"_el);
        loader->set("optional"_el, "X"_el);
        REQUIRE_EQUAL(environment->render("optional-page"_el), "AXB"_el);
        loader->remove("optional"_el);
        REQUIRE_EQUAL(environment->render("optional-page"_el), "AB"_el);
        loader->remove("leaf"_el);
        REQUIRE_THROWS_AS(RenderError, environment->render("page"_el));
    }

    void testFailedReloadRecoveryAndSharedDependency() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "{% include \"left\" %}|{% include \"right\" %}"_el);
        loader->set("left"_el, "L{% include \"shared\" %}"_el);
        loader->set("right"_el, "R{% include \"shared\" %}"_el);
        loader->set("shared"_el, "old"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();

        REQUIRE_EQUAL(environment->render("page"_el), "Lold|Rold"_el);
        loader->set("shared"_el, "{% if %}"_el);
        REQUIRE_THROWS_AS(RenderError, environment->render("page"_el));
        loader->set("shared"_el, "new"_el);
        REQUIRE_EQUAL(environment->render("page"_el), "Lnew|Rnew"_el);
    }

    void testConcurrentGenerationPublication() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "A{% include \"child\" %}B"_el);
        loader->set("child"_el, "old"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();
        REQUIRE_EQUAL(environment->render("page"_el), "AoldB"_el);

        auto failed = std::atomic_bool{false};
        auto workers = std::vector<std::thread>{};
        for (auto worker = 0; worker < 4; ++worker) {
            workers.emplace_back([&]() -> void {
                for (auto iteration = 0; iteration < 25; ++iteration) {
                    const auto result = environment->render("page"_el);
                    if (result != "AoldB"_el && result != "AnewB"_el) {
                        failed = true;
                    }
                }
            });
        }
        loader->set("child"_el, "new"_el);
        for (auto &worker : workers) {
            worker.join();
        }
        REQUIRE_FALSE(failed.load());
        REQUIRE_EQUAL(environment->render("page"_el), "AnewB"_el);
    }

private:
    [[nodiscard]] static auto environmentFor(const LoaderPtr &loader) -> EnvironmentPtr {
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment;
    }

    static void addDepthChain(
        const std::shared_ptr<erbsland::test::MemoryLayoutLoader> &loader, const std::size_t edges) {
        for (auto index = std::size_t{}; index < edges; ++index) {
            const auto name = el::text::StringFormat{"depth/{}"_el}.build(index);
            const auto next = el::text::StringFormat{"depth/{}"_el}.build(index + 1U);
            loader->set(name, el::text::StringFormat{"{{% include \"{}\" %}}"_el}.build(next));
        }
        loader->set(el::text::StringFormat{"depth/{}"_el}.build(edges), "end"_el);
    }
};
