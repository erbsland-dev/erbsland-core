// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Compiler Engine LoopState Tokenizer)
class RenderIterationTest final : public el::UnitTest {
public:
    void testListIterationAndMetadata() {
        const auto context = Context{}.set("items"_el, ValueList{"alpha"_el, "beta"_el, "gamma"_el});
        REQUIRE_EQUAL(
            render(
                "{% for item in items %}"
                "{{ loop.index }}/{{ loop.index0 }}/{{ loop.revindex }}/{{ loop.revindex0 }}/"
                "{{ loop.first }}/{{ loop.last }}/{{ loop.length }}={{ item }};"
                "{% endfor %}"_el,
                context),
            "1/0/3/2/true/false/3=alpha;2/1/2/1/false/false/3=beta;3/2/1/0/false/true/3=gamma;"_el);
        REQUIRE_EQUAL(
            render("A{% for item in items %}{{ item }}{% endfor %}Z"_el, Context{}.set("items"_el, ValueList{})),
            "AZ"_el);
        REQUIRE_EQUAL(
            render(
                "{% for item in items %}{{ loop.first }}:{{ loop.last }}={{ item }}{% endfor %}"_el,
                Context{}.set("items"_el, ValueList{"only"_el})),
            "true:true=only"_el);
    }

    void testOrderedMapIteration() {
        auto values = ValueMap{};
        values.set("zulu"_el, 3).set("alpha"_el, 1).set("middle"_el, 2);
        const auto context = Context{}.set("values"_el, values);

        REQUIRE_EQUAL(
            render("{% for key, value in values %}{{ key }}={{ value }};{% endfor %}"_el, context),
            "alpha=1;middle=2;zulu=3;"_el);
    }

    void testNestedScopesAndAssignments() {
        const auto rows = ValueList{ValueList{"a"_el, "b"_el}, ValueList{"c"_el}};
        const auto context = Context{}.set("rows"_el, rows).set("item"_el, "outer"_el).set("saved"_el, "root"_el);

        REQUIRE_EQUAL(
            render(
                "{% for row in rows %}O{{ loop.index }}["
                "{% set saved = \"iteration\" %}"
                "{% for item in row %}I{{ loop.index }}={{ item }}:{{ saved }};{% set saved = \"inner\" %}{% endfor %}"
                "]{{ saved }};{% endfor %}{{ item }}|{{ saved }}"_el,
                context),
            "O1[I1=a:iteration;I2=b:iteration;]iteration;O2[I1=c:iteration;]iteration;outer|root"_el);
    }

    void testIterableExpressionCallbackAndFilter() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, "{% for item in items | identity %}{{ item }}{% endfor %}"_el);
        const auto environment = Environment::create();
        environment->addFilter("identity"_el, [](const ValueList &values) -> Value {
            return values.getRefOrThrow(el::unit::ItemIndex::zero());
        });
        environment->addLayoutLoader(loader);
        auto evaluations = std::size_t{};
        const auto context = Context{}.set("items"_el, ValueCallbackFn{[&evaluations]() -> Value {
            ++evaluations;
            return ValueList{"x"_el, "y"_el};
        }});

        REQUIRE_EQUAL(environment->render("page"_el, context), "xy"_el);
        REQUIRE_EQUAL(evaluations, std::size_t{1U});
        REQUIRE_EQUAL(environment->render("page"_el, context), "xy"_el);
        REQUIRE_EQUAL(evaluations, std::size_t{2U});
    }

    void testSyntaxFailures() {
        REQUIRE_THROWS_AS(RenderError, render("{% for %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item items %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item in %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for a, in values %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for a, b, c in values %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for a, a in values %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for loop in values %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for if in values %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item, endfor in values %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item in values %}{% set loop = 1 %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item in values %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item in values %}{% endfor extra %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item in values %}{% else %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% if true %}{% for item in values %}{% endif %}{% endfor %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% for item in values %}{% if true %}{% endfor %}{% endif %}"_el));
    }

    void testRuntimeTypeFailuresAndLocation() {
        WITH_CONTEXT(requireRuntimeFailure("before\n{% for item in value %}{% endfor %}"_el, Value{7}, 1U));
        WITH_CONTEXT(requireRuntimeFailure("{% for key, value in value %}{% endfor %}"_el, ValueList{1, 2}, 0U));
        auto map = ValueMap{};
        map.set("one"_el, 1);
        WITH_CONTEXT(requireRuntimeFailure("{% for item in value %}{% endfor %}"_el, Value{map}, 0U));
        WITH_CONTEXT(requireRuntimeFailure("{% for item in value %}{% endfor %}"_el, Value{}, 0U));
        WITH_CONTEXT(requireRuntimeFailure(
            "{% for item in value %}{% endfor %}"_el,
            ValueCallbackFn{[]() -> Value { return Value{"not-a-list"_el}; }},
            0U));
    }

private:
    [[nodiscard]] static auto render(const el::text::String &text, const Context &context = {}) -> el::text::String {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, text);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment->render("page"_el, context);
    }

    void requireRuntimeFailure(const el::text::String &layout, Value value, const std::size_t expectedLine) {
        try {
            static_cast<void>(render(layout, Context{}.set("value"_el, std::move(value))));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().location().line().toSizeT(), expectedLine);
        }
    }
};
