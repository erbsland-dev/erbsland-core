// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Context)
class RenderContextTest final : public el::UnitTest {
public:
    void testNamedValues() {
        auto context = Context{};
        REQUIRE_FALSE(context.contains("name"_el));
        REQUIRE(context.get("name"_el).isNull());

        context.set("name"_el, "Ada"_el).set("active"_el, true);
        REQUIRE(context.contains("name"_el));
        REQUIRE_EQUAL(context.get("name"_el).asText(), "Ada"_el);
        REQUIRE(context.get("active"_el).asBoolean());

        const auto copy = context;
        context.set("name"_el, "Grace"_el);
        REQUIRE_EQUAL(copy.get("name"_el).asText(), "Ada"_el);
        REQUIRE_EQUAL(context.get("name"_el).asText(), "Grace"_el);
    }
};
