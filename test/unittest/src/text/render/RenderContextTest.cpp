// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Literals.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Context)
class RenderContextTest final : public el::UnitTest {
private:
    static constexpr auto cActiveKey = "active"_el;
    static constexpr auto cNameKey = "name"_el;

public:
    void testNamedValues() {
        auto context = Context{};
        REQUIRE_FALSE(context.contains(cNameKey));
        REQUIRE(context.get(cNameKey).isNull());

        context.set(cNameKey, "Ada"_el).set(cActiveKey, true);
        REQUIRE(context.contains(cNameKey));
        REQUIRE_EQUAL(context.get(cNameKey).asText(), "Ada"_el);
        REQUIRE(context.get(cActiveKey).asBoolean());

        const auto copy = context;
        context.set(cNameKey, "Grace"_el);
        REQUIRE_EQUAL(copy.get(cNameKey).asText(), "Ada"_el);
        REQUIRE_EQUAL(context.get(cNameKey).asText(), "Grace"_el);
    }
};
