// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Compiler Engine)
class RenderStatementTest final : public el::UnitTest {
private:
    static constexpr auto cPageLayout = "page"_el;
    static constexpr auto cValueKey = "value"_el;

public:
    void testIfElifElseAndNesting() {
        REQUIRE_EQUAL(render("{% if true %}yes{% else %}no{% endif %}"_el), "yes"_el);
        REQUIRE_EQUAL(render("{% if false %}a{% elif 2 > 1 %}b{% elif true %}c{% else %}d{% endif %}"_el), "b"_el);
        REQUIRE_EQUAL(render("{% if true %}A{% if false %}x{% else %}B{% endif %}C{% endif %}"_el), "ABC"_el);
        REQUIRE_EQUAL(render("x{% if false %}{% endif %}y"_el), "xy"_el);
    }

    void testSetScopeAndPrecedence() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(
            cPageLayout,
            "{{ value }}|{% set value = \"render\" %}{{ value }}|"
            "{% if true %}{% set branch = 7 %}{% endif %}{{ branch }}"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->setGlobalContext(Context{}.set(cValueKey, "global"_el));
        const auto local = Context{}.set(cValueKey, "local"_el);

        REQUIRE_EQUAL(environment->render(cPageLayout, local), "render|render|7"_el);
        REQUIRE_EQUAL(local.get(cValueKey).asText(), "local"_el);
        REQUIRE_FALSE(local.contains("branch"_el));
    }

    void testStatementDelimiterInsideString() {
        REQUIRE_EQUAL(render("{% set value = \"inside %} delimiter\" %}{{ value }}"_el), "inside %} delimiter"_el);
    }

    void testConditionalDelimiterInsideString() {
        const auto context = Context{}.set("a"_el, "%}"_el);
        REQUIRE_EQUAL(render("<p>{% if a == \"%}\" %}yes{% endif %}</p>"_el, context), "<p>yes</p>"_el);
        try {
            static_cast<void>(render("<p>{% if a == \"%}\" %}</p>"_el, context));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().description(), "An 'if' statement has no matching 'endif'."_el);
        }
    }

    void testSkippedAssignmentsAndConditions() {
        REQUIRE_EQUAL(
            render(
                "{% set selected = \"start\" %}{% if false %}{% set selected = \"bad\" %}"
                "{% elif true %}{% set selected = \"good\" %}{% endif %}{{ selected }}"_el),
            "good"_el);
    }

    void testMalformedStatements() {
        REQUIRE_THROWS_AS(RenderError, render("{% if true %}missing"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% else %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% endif %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% if true %}{% else %}{% elif true %}{% endif %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% if true %}{% else %}{% else %}{% endif %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% set item.value = 1 %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% set item %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% set true = 1 %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% unknown %}"_el));
    }

private:
    [[nodiscard]] static auto render(const el::text::String &text, const Context &context = {}) -> el::text::String {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(cPageLayout, text);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment->render(cPageLayout, context);
    }
};
