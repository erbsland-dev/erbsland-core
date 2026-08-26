// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/text/ToString.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(CompiledBlock CompiledExtends CompiledLayout Compiler Engine Environment ExpressionCompiler)
class RenderInheritanceTest final : public el::UnitTest {
public:
    void testFallbackOverridesAndMultiLevelSuper() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(
            "base"_el, "A{% block title %}base title{% endblock %}|{% block body %}base body{% endblock %}Z"_el);
        loader->set("middle"_el, "{% extends \"base\" %}{% block body %}middle({{ super() }}){% endblock %}"_el);
        loader->set(
            "page"_el,
            "{% extends \"middle\" %}{% block body %}page({{ super() }})/{{ super.super() }}{% endblock %}"_el);

        REQUIRE_EQUAL(environmentFor(loader)->render("page"_el), "Abase title|page(middle(base body))/base bodyZ"_el);
    }

    void testHoistedSetupOrderAndConditionalAssignments() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(
            "base"_el,
            "{{ value }}|{% set value = \"base\" %}{% set trace = \"base\" %}"
            "{% block body %}{{ value }}/{{ trace }}{% endblock %}"_el);
        loader->set(
            "middle"_el,
            "{% set trace = value %}{% extends \"base\" %}{% set value = \"middle\" %}"
            "{% block body %}{% if enabled %}{% set value = \"conditional\" %}{% endif %}{{ super() }}{% endblock %}"_el);
        loader->set("page"_el, "{% extends \"middle\" %}{% set value = \"page\" %}"_el);

        REQUIRE_EQUAL(
            environmentFor(loader)->render("page"_el, Context{}.set("enabled"_el, false)), "page|page/base"_el);
        REQUIRE_EQUAL(
            environmentFor(loader)->render("page"_el, Context{}.set("enabled"_el, true)), "page|conditional/base"_el);
    }

    void testNestedBlocksAndVisibleLoopScope() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(
            "base"_el,
            "{% for item in items %}[{% block row %}{{ loop.index }}={{ item }}:"
            "{% block detail %}base{% endblock %}{% endblock %}]{% endfor %}"_el);
        loader->set(
            "page"_el,
            "{% extends \"base\" %}{% block detail %}detail-{{ item }}{% endblock %}"
            "{% block row %}{{ super() }}+{{ item }}{% endblock %}"_el);

        REQUIRE_EQUAL(
            environmentFor(loader)->render("page"_el, Context{}.set("items"_el, ValueList{"a"_el, "b"_el})),
            "[1=a:detail-a+a][2=b:detail-b+b]"_el);
    }

    void testCapturedSuperExpressionsAndFilter() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("base"_el, "{% block body %}base{% endblock %}"_el);
        loader->set(
            "page"_el,
            "{% extends \"base\" %}{% block body %}{% set saved = super() %}"
            "{{ super() | frame }}|{{ saved }}|{{ super() == \"base\" }}{% endblock %}"_el);
        const auto environment = environmentFor(loader);
        environment->addFilter("frame"_el, [](const ValueList &values) -> Value {
            return el::text::StringList{"["_el, values.getRefOrThrow(el::unit::ItemIndex::zero()).asText(), "]"_el}
                .join();
        });

        REQUIRE_EQUAL(environment->render("page"_el), "[base]|base|true"_el);
    }

    void testStrictGrammarAndForbiddenExtendingContent() {
        const auto invalid = std::vector<el::text::String>{
            "{% extends parent %}"_el,
            "{% extends \"base\" with context %}"_el,
            "{% extends \"base\" %}{% extends \"base\" %}"_el,
            "{% if true %}{% extends \"base\" %}{% endif %}"_el,
            "text{% extends \"base\" %}"_el,
            "{% extends \"base\" %}text"_el,
            "{{ value }}{% extends \"base\" %}"_el,
            "{% extends \"base\" %}{% include \"base\" %}"_el,
            "{% extends \"base\" %}{% if true %}{% endif %}"_el,
            "{% block %}{% endblock %}"_el,
            "{% block bad-name %}{% endblock %}"_el,
            "{% block body extra %}{% endblock %}"_el,
            "{% block body %}{% endblock body %}"_el,
            "{% block body %}{% endblock %}{% block body %}{% endblock %}"_el,
            "{% block body %}{% if true %}{% endblock %}{% endif %}"_el,
            "{% endblock %}"_el,
            "{{ super() }}"_el,
            "{% block body %}{{ super(1) }}{% endblock %}"_el,
            "{% block body %}{{ super.super.super() }}{% endblock %}"_el,
        };
        for (const auto &source : invalid) {
            WITH_CONTEXT(requireSyntaxFailure(source));
        }

        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("base"_el, "{% block body %}base{% endblock %}"_el);
        loader->set(
            "valid"_el,
            " \n{# comment #}{% set value = \"ok\" %}{% extends \"base\" %} \n"
            "{% block body %}{{ value }}{% endblock %}"_el);
        REQUIRE_EQUAL(environmentFor(loader)->render("valid"_el), "ok"_el);
    }

    void testMissingSuperAndParents() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("missing-parent"_el, "{% extends \"absent\" %}"_el);
        loader->set("base"_el, "{% block body %}base{% endblock %}"_el);
        loader->set("deep-super"_el, "{% extends \"base\" %}{% block body %}{{ super.super() }}{% endblock %}"_el);

        REQUIRE_THROWS_AS(RenderError, environmentFor(loader)->render("missing-parent"_el));
        try {
            static_cast<void>(environmentFor(loader)->render("deep-super"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().layout(), "deep-super"_el);
        }
    }

    void testInheritanceAndMixedDependencyCyclesAndDepth() {
        const auto inheritance = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        inheritance->set("a"_el, "{% extends \"b\" %}"_el);
        inheritance->set("b"_el, "{% extends \"a\" %}"_el);
        REQUIRE_THROWS_AS(RenderError, environmentFor(inheritance)->render("a"_el));

        const auto mixed = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        mixed->set("a"_el, "{% extends \"b\" %}"_el);
        mixed->set("b"_el, "{% include \"a\" %}"_el);
        REQUIRE_THROWS_AS(RenderError, environmentFor(mixed)->render("a"_el));

        const auto depth = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        addDepthChain(depth, 32U);
        REQUIRE_EQUAL(environmentFor(depth)->render("depth/0"_el), "end"_el);
        addDepthChain(depth, 33U);
        try {
            static_cast<void>(environmentFor(depth)->render("depth/0"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Limit);
        }
    }

    void testIncludeInteractionAndDiagnosticFrames() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("shell"_el, "S{% block body %}base{% endblock %}E"_el);
        loader->set("partial"_el, "P{{ item }}"_el);
        loader->set("page"_el, "{% extends \"shell\" %}{% block body %}{% include \"partial\" %}{% endblock %}"_el);
        REQUIRE_EQUAL(environmentFor(loader)->render("page"_el, Context{}.set("item"_el, "X"_el)), "SPXE"_el);

        loader->set("root"_el, "{% extends \"middle\" %}"_el);
        loader->set("middle"_el, "{% extends \"leaf\" %}"_el);
        loader->set("leaf"_el, "{{ value }}"_el);
        try {
            static_cast<void>(environmentFor(loader)->render("root"_el, Context{}.set("value"_el, ValueList{})));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().layout(), "leaf"_el);
            REQUIRE_EQUAL(error.context().frames().count(), el::unit::ItemCount{2U});
            REQUIRE_EQUAL(error.context().frames().get(el::unit::ItemIndex{0U}), "root:1:4"_el);
            REQUIRE_EQUAL(error.context().frames().get(el::unit::ItemIndex{1U}), "middle:1:4"_el);
        }
    }

    void testTransitiveReloadFailureRecoveryAndPublication() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("base"_el, "A{% block body %}old{% endblock %}Z"_el);
        loader->set("middle"_el, "{% extends \"base\" %}"_el);
        loader->set("page"_el, "{% extends \"middle\" %}{% block body %}{{ super() }}!{% endblock %}"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->enableAutoReload();

        REQUIRE_EQUAL(environment->render("page"_el), "Aold!Z"_el);
        loader->set("base"_el, "A{% block body %}new{% endblock %}Z"_el);
        REQUIRE_EQUAL(environment->render("page"_el), "Anew!Z"_el);
        loader->set("base"_el, "{% if %}"_el);
        REQUIRE_THROWS_AS(RenderError, environment->render("page"_el));
        loader->set("base"_el, "A{% block body %}fixed{% endblock %}Z"_el);
        REQUIRE_EQUAL(environment->render("page"_el), "Afixed!Z"_el);

        auto failed = std::atomic_bool{false};
        auto workers = std::vector<std::thread>{};
        for (auto worker = 0; worker < 4; ++worker) {
            workers.emplace_back([&]() -> void {
                for (auto iteration = 0; iteration < 25; ++iteration) {
                    const auto result = environment->render("page"_el);
                    if (result != "Afixed!Z"_el && result != "Alast!Z"_el) {
                        failed = true;
                    }
                }
            });
        }
        loader->set("base"_el, "A{% block body %}last{% endblock %}Z"_el);
        for (auto &worker : workers) {
            worker.join();
        }
        REQUIRE_FALSE(failed.load());
        REQUIRE_EQUAL(environment->render("page"_el), "Alast!Z"_el);
    }

    void testParentLoaderPriority() {
        const auto lowPriority = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        const auto highPriority = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        lowPriority->set("base"_el, "low:{% block body %}base{% endblock %}"_el);
        highPriority->set("base"_el, "high:{% block body %}base{% endblock %}"_el);
        lowPriority->set("page"_el, "{% extends \"base\" %}{% block body %}page{% endblock %}"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(lowPriority, 0);
        environment->addLayoutLoader(highPriority, 10);

        REQUIRE_EQUAL(environment->render("page"_el), "high:page"_el);
    }

private:
    [[nodiscard]] static auto environmentFor(const LoaderPtr &loader) -> EnvironmentPtr {
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment;
    }

    void requireSyntaxFailure(const el::text::String &source) {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("base"_el, "{% block body %}base{% endblock %}"_el);
        loader->set("page"_el, source);
        try {
            static_cast<void>(environmentFor(loader)->render("page"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Syntax);
        }
    }

    static void addDepthChain(
        const std::shared_ptr<erbsland::test::MemoryLayoutLoader> &loader, const std::size_t edges) {
        for (auto index = std::size_t{}; index < edges; ++index) {
            const auto name = el::text::StringFormat{"depth/{}"_el}.build(index);
            const auto next = el::text::StringFormat{"depth/{}"_el}.build(index + 1U);
            loader->set(name, el::text::StringFormat{"{{% extends \"{}\" %}}"_el}.build(next));
        }
        loader->set(el::text::StringFormat{"depth/{}"_el}.build(edges), "end"_el);
    }
};
