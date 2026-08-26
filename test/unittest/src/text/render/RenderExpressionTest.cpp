// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "MemoryLayoutLoader.hpp"

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(ExpressionCompiler Engine)
class RenderExpressionTest final : public el::UnitTest {
public:
    void testScalarLiteralsAndEscapes() {
        REQUIRE_EQUAL(
            render(
                "{{ -9223372036854775808 }}|{{ 42 }}|{{ 1.5 }}|{{ 1e2 }}|{{ true }}|{{ false }}|"
                "{{ \"a\\n\\t\\u0042\" }}|{{ 'it\\'s' }}|{{ \"inside }} delimiter\" }}"_el),
            "-9223372036854775808|42|1.5|100|true|false|a\n\tB|it's|inside }} delimiter"_el);
    }

    void testPrecedenceGroupingAndOperandLogic() {
        REQUIRE_EQUAL(
            render(
                "{{ false or \"fallback\" }}|{{ true and 7 }}|{{ not false }}|"
                "{{ (false or true) and \"yes\" }}|{{ false or true and false }}"_el),
            "fallback|7|true|yes|false"_el);
    }

    void testComparisons() {
        REQUIRE_EQUAL(
            render(
                "{{ 1 == 1.0 }}|{{ 9007199254740993 == 9007199254740992.0 }}|"
                "{{ \"a\" < \"b\" }}|{{ true == 1 }}|{{ 1 != \"1\" }}|{{ -2 < -1.5 }}"_el),
            "true|false|true|false|true|true"_el);
    }

    void testDottedNamesAndShortCircuit() {
        auto calls = std::atomic_size_t{0U};
        auto profile = ValueMap{};
        profile.set("name"_el, "Ada"_el);
        auto user = ValueMap{};
        user.set("profile"_el, profile);
        user.set("true"_el, "reserved member"_el);
        const auto context = Context{}.set("user"_el, user).set("skipped"_el, ValueCallbackFn{[&calls]() -> Value {
            ++calls;
            return "unexpected"_el;
        }});

        REQUIRE_EQUAL(
            render("{{ user.profile.name }}|{{ user.true }}|{{ true or skipped }}|{{ false and skipped }}"_el, context),
            "Ada|reserved member|true|false"_el);
        REQUIRE_EQUAL(calls.load(), std::size_t{0U});
    }

    void testSyntaxErrors() {
        REQUIRE_THROWS_AS(RenderError, render("{{ 1 < 2 < 3 }}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{{ 9223372036854775808 }}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{{ \"unterminated }}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{{ +9223372036854775808 }}"_el));
        REQUIRE_THROWS_AS(RenderError, render("{{ \"\\uFEFF\" }}"_el));
    }

    void testRuntimeErrorsAndLocations() {
        try {
            static_cast<void>(render("line\n{{ 1 < true }}"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Runtime);
            REQUIRE_EQUAL(error.context().location().line(), el::unit::LineIndex{1U});
            REQUIRE_EQUAL(error.context().location().column(), el::unit::ColumnIndex{5U});
        }
        REQUIRE_THROWS_AS(
            RenderError,
            render("{{ left == right }}"_el, Context{}.set("left"_el, ValueList{}).set("right"_el, ValueList{})));
    }

private:
    [[nodiscard]] static auto render(const el::text::String &text, const Context &context = {}) -> el::text::String {
        const auto loader = std::make_shared<erbsland::test::MemoryLayoutLoader>();
        loader->set("page"_el, text);
        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        return environment->render("page"_el, context);
    }
};
