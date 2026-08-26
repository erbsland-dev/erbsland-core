// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/math/SaturatingMath.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(ExpressionCompiler Engine built_in_filters EnvironmentOptions RenderLimits)
class RenderLanguageCompletionTest final : public el::UnitTest {
public:
    void testSaturatingArithmeticAndTypedDivision() {
        constexpr auto cMinimum = std::numeric_limits<int64_t>::min();
        constexpr auto cMaximum = std::numeric_limits<int64_t>::max();
        REQUIRE_EQUAL(
            render(
                "{{ 1 + 2 * 3 }}|{{ 9223372036854775807 + 1 }}|{{ -9223372036854775808 - 1 }}|"
                "{{ -9223372036854775808 / -1 }}|{{ 7 / 2 }}|{{ 7 / 2.0 }}|{{ 1 / 0 }}|{{ 1.0 / -0.0 }}"_el),
            "7|9223372036854775807|-9223372036854775808|9223372036854775807|3|3.5||"_el);
        REQUIRE_EQUAL(el::math::saturatingAdd(cMaximum, int64_t{1}), cMaximum);
        requireIntegerOperation("a + b"_el, cMaximum, 1, el::math::saturatingAdd(cMaximum, int64_t{1}));
        requireIntegerOperation("a + b"_el, cMinimum, -1, el::math::saturatingAdd(cMinimum, int64_t{-1}));
        requireIntegerOperation("a - b"_el, cMaximum, -1, el::math::saturatingSubtract(cMaximum, int64_t{-1}));
        requireIntegerOperation("a - b"_el, cMinimum, 1, el::math::saturatingSubtract(cMinimum, int64_t{1}));
        requireIntegerOperation("a * b"_el, cMaximum, 2, el::math::saturatingMultiply(cMaximum, int64_t{2}));
        requireIntegerOperation("a * b"_el, cMinimum, 2, el::math::saturatingMultiply(cMinimum, int64_t{2}));
        requireIntegerOperation("a * b"_el, cMinimum, -1, el::math::saturatingMultiply(cMinimum, int64_t{-1}));
        requireIntegerOperation("a / b"_el, cMinimum, -1, el::math::saturatingDivide(cMinimum, int64_t{-1}));
        requireIntegerOperation("a / b"_el, cMinimum, 1, el::math::saturatingDivide(cMinimum, int64_t{1}));
        requireIntegerOperation("a / b"_el, cMaximum, -1, el::math::saturatingDivide(cMaximum, int64_t{-1}));
        REQUIRE_EQUAL(render("{{ -a }}"_el, Context{}.set("a"_el, cMinimum)), el::text::String::fromInteger(cMaximum));
    }

    void testCollectionsMembershipTestsAndConcatenation() {
        REQUIRE_EQUAL(
            render(
                "{{ [3, 1, 2,] | sort | join(',') }}|{{ {'b': 2, 'a': 1,} | keys | join(',') }}|"
                "{{ 'bc' in 'abcd' }}|{{ 2 in [1, 2] }}|{{ 'a' in {'a': 1} }}|{{ 4 not in [1, 2] }}|"
                "{{ none is null }}|{{ 2 is number }}|{{ [1] is sequence }}|{{ 3 is odd }}|"
                "{{ none ~ ':' ~ true ~ ':' ~ 4 }}"_el),
            "1,2,3|a,b|true|true|true|true|true|true|true|true|:true:4"_el);
    }

    void testFilterArgumentsBuiltInsAndOverrides() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(
            "page"_el,
            "{{ 'a-b-a' | replace('a', 'x') }}|{{ '  AbC  ' | trim | lower }}|"
            "{{ [1, 2, 3] | sum(4) }}|{{ none | default('missing') }}|"
            "{{ {'name': 'Ada'} | tojson }}|{{ 'mixed' | lower }}|{{ 'x' | combine('a', 'b') }}"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        environment->addFilter("lower"_el, [](const ValueList &) -> Value { return "override"_el; });
        environment->addFilter("combine"_el, [](const ValueList &values) -> Value {
            auto result = el::text::StringList{};
            values.forEach([&result](const Value &value) -> void { result.append(value.toString()); });
            return result.join("-"_el);
        });
        REQUIRE_EQUAL(environment->render("page"_el), "x-b-x|override|10|missing|{\"name\":\"Ada\"}|override|x-a-b"_el);
    }

    void testEveryBuiltInFilter() {
        auto services = ValueList{};
        services.append(ValueMap{}.set("name"_el, "API"_el));
        services.append(ValueMap{}.set("name"_el, "Storage"_el));
        const auto context = Context{}.set("services"_el, services);
        REQUIRE_EQUAL(
            render(
                "{{ 'hELLO world' | capitalize }}|{{ 'MiXeD' | lower }}|{{ 'MiXeD' | upper }}|"
                "{{ '--x--' | trim('-') }}|{{ 'a-b-a' | replace('a', 'x') }}|"
                "{{ [1, 2] | first }}|{{ [1, 2] | last }}|{{ [] | first is none }}|{{ [] | last is none }}|"
                "{{ services | join(',', 'name') }}|{{ 'abc' | length }}|{{ [1, 2] | count }}|"
                "{{ '' | reverse }}|{{ 'A€😀' | reverse }}|{{ [1, 2, 3] | reverse | join(',') }}|"
                "{{ [1, 3, 2] | sort(true, true) | join(',') }}|"
                "{{ {'b': 2, 'a': 1} | keys | join(',') }}|{{ {'b': 2, 'a': 1} | values | join(',') }}|"
                "{{ {'a': 1} | items | length }}|{{ (-9223372036854775808) | abs }}|"
                "{{ 1.21 | round(1, 'ceil') }}|{{ [1, 2, 3] | sum(4) }}|"
                "{{ [3, 1, 2] | min }}|{{ [3, 1, 2] | max }}|{{ false | d('fallback', true) }}|"
                "{{ {'a': 1} | tojson(0) | safe }}"_el,
                context),
            "Hello world|mixed|MIXED|x|x-b-x|1|2|true|true|API,Storage|3|2||😀€A|3,2,1|3,2,1|"
            "a,b|1,2|1|9223372036854775807|1.3|10|1|3|fallback|{\"a\":1}"_el);
    }

    void testLoopElseAndScopes() {
        REQUIRE_EQUAL(
            render(
                "{% for x in [] %}bad{% else %}empty{% endfor %}|"
                "{% for x in [1, 2] %}{{ x }}{% else %}bad{% endfor %}|"
                "{% for row in [[], [3]] %}{% for x in row %}{{ x }}{% else %}-{% endfor %}{% endfor %}"_el),
            "empty|12|-3"_el);
    }

    void testAutomaticAndExplicitEscaping() {
        REQUIRE_EQUAL(
            render("{{ value }}"_el, Context{}.set("value"_el, "<&\""_el), "page.html"_el), "&lt;&amp;&quot;"_el);
        REQUIRE_EQUAL(render("{{ value }}"_el, Context{}.set("value"_el, "a*b"_el), "page.md"_el), "a\\*b"_el);
        REQUIRE_EQUAL(render("{{ value | safe }}"_el, Context{}.set("value"_el, "<&"_el), "page.html"_el), "<&"_el);
        REQUIRE_EQUAL(render("{{ value | escape(json) }}"_el, Context{}.set("value"_el, "\"\n"_el)), "\\\"\\n"_el);

        auto options = EnvironmentOptions{};
        options.setAutomaticEscapingEnabled(false);
        REQUIRE_EQUAL(render("{{ value }}"_el, Context{}.set("value"_el, "<&"_el), "page.html"_el, options), "<&"_el);
        REQUIRE_EQUAL(
            render("{{ value | escape }}"_el, Context{}.set("value"_el, "<&"_el), "page.html"_el, options),
            "&lt;&amp;"_el);
    }

    void testStrictFailuresAndLimits() {
        REQUIRE_THROWS_AS(RenderError, render("{{ [1] + [2] }}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{{ {'a': 1, 'a': 2} }}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{{ 1 in 2 }}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{% set x = 'a' | safe %}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{{ 'x' | replace('x', 'y', 'z') }}"_el));

        auto options = EnvironmentOptions{};
        options.setRenderLimits(RenderLimits{}.setGeneratedOutputBytes(3U));
        REQUIRE_THROWS_AS(RenderError, render("abcd"_el, {}, "page"_el, options));
        options.setRenderLimits(RenderLimits{}.setExecutedInstructions(1U));
        REQUIRE_THROWS_AS(RenderError, render("{{ 1 + 2 }}"_el, {}, "page"_el, options));
    }

    void testEnvironmentOptionsAndAllLimitValidation() {
        auto options = EnvironmentOptions{};
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.html"_el), el::text::EscapeFormat::Html);
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.xml"_el), el::text::EscapeFormat::Xml);
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.elcl"_el), el::text::EscapeFormat::Config);
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.md"_el), el::text::EscapeFormat::Markdown);
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.json"_el), el::text::EscapeFormat::Json);
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.txt"_el), el::text::EscapeFormat::None);
        options.setEscapeFormatForSuffix(".safe.html"_el, el::text::EscapeFormat::Json);
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.safe.html"_el), el::text::EscapeFormat::Json);
        REQUIRE(options.removeEscapeFormatForSuffix(".safe.html"_el));
        REQUIRE_FALSE(options.removeEscapeFormatForSuffix(".safe.html"_el));
        options.clearEscapeFormats();
        REQUIRE_EQUAL(options.escapeFormatForLayout("page.html"_el), el::text::EscapeFormat::None);
        REQUIRE_THROWS_AS(
            el::err::ParameterError, options.setEscapeFormatForSuffix("html"_el, el::text::EscapeFormat::Html));

        auto limits = RenderLimits{};
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setGeneratedOutputBytes(0U));
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setExecutedInstructions(0U));
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setNestingDepth(0U));
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setCallbackDepth(0U));
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setLexicalScopeDepth(0U));
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setCallFrameDepth(0U));
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setValueStackDepth(0U));
        REQUIRE_THROWS_AS(el::err::ParameterError, limits.setStaticDependencyDepth(0U));
    }

    void testEveryRuntimeLimit() {
        auto options = EnvironmentOptions{};
        options.setRenderLimits(RenderLimits{}.setNestingDepth(1U));
        REQUIRE_THROWS_AS(
            RenderError,
            render("{% for a in [1] %}{% for b in [1] %}x{% endfor %}{% endfor %}"_el, {}, "page"_el, options));

        options.setRenderLimits(RenderLimits{}.setLexicalScopeDepth(1U));
        REQUIRE_THROWS_AS(
            RenderError,
            render("{% for a in [1] %}{% for b in [1] %}x{% endfor %}{% endfor %}"_el, {}, "page"_el, options));

        options.setRenderLimits(RenderLimits{}.setValueStackDepth(1U));
        REQUIRE_THROWS_AS(RenderError, render("{{ [1, 2] | first }}"_el, {}, "page"_el, options));

        options.setRenderLimits(RenderLimits{}.setCallFrameDepth(1U));
        REQUIRE_THROWS_AS(RenderError, render("{% block body %}x{% endblock %}"_el, {}, "page"_el, options));

        const auto callback =
            ValueCallbackFn{[]() -> Value { return ValueCallbackFn{[]() -> Value { return "done"_el; }}; }};
        options.setRenderLimits(RenderLimits{}.setCallbackDepth(1U));
        REQUIRE_THROWS_AS(
            RenderError, render("{{ value }}"_el, Context{}.set("value"_el, callback), "page"_el, options));

        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("root"_el, "{% include 'a' %}"_el);
        loader->set("a"_el, "{% include 'b' %}"_el);
        loader->set("b"_el, "x"_el);
        options.setRenderLimits(RenderLimits{}.setStaticDependencyDepth(1U));
        const auto environment = Environment::create(options);
        environment->addLayoutLoader(loader);
        REQUIRE_THROWS_AS(RenderError, environment->render("root"_el));
    }

    void testEscapingAcrossIncludesInheritanceAndSuper() {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("base.html"_el, "<body>{% block body %}{{ value }}{% endblock %}</body>"_el);
        loader->set(
            "page.html"_el,
            "{% extends 'base.html' %}{% block body %}{{ super() }}|{{ value }}|{{ super() | upper }}{% endblock %}"_el);
        loader->set("fragment.md"_el, "{{ value }}"_el);
        loader->set("include.html"_el, "{% include 'fragment.md' %}|{{ value }}"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        REQUIRE_EQUAL(
            environment->render("page.html"_el, Context{}.set("value"_el, "<x>"_el)),
            "<body>&lt;x&gt;|&lt;x&gt;|&amp;LT;X&amp;GT;</body>"_el);
        REQUIRE_EQUAL(
            environment->render("include.html"_el, Context{}.set("value"_el, "a*b<&"_el)),
            "a\\*b\\<\\&|a*b&lt;&amp;"_el);
    }

private:
    void requireIntegerOperation(
        const el::text::String &expression, const int64_t left, const int64_t right, const int64_t expected) {
        REQUIRE_EQUAL(
            render(
                el::text::StringFormat{"{{{{ {} }}}}"_el}.build(expression),
                Context{}.set("a"_el, left).set("b"_el, right)),
            el::text::String::fromInteger(expected));
    }

    [[nodiscard]] static auto render(
        const el::text::String &text,
        const Context &context = {},
        const el::text::String &layout = "page"_el,
        EnvironmentOptions options = {}) -> el::text::String {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set(layout, text);
        const auto environment = Environment::create(std::move(options));
        environment->addLayoutLoader(loader);
        return environment->render(layout, context);
    }
};
