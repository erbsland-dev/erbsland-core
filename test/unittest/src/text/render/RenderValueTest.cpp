// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Value.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>

using namespace el::text::literals;
using namespace el::text::render;

TESTED_TARGETS(Value ValueType)
class RenderValueTest final : public el::UnitTest {
public:
    void testScalars() {
        const auto nullValue = Value{};
        const auto boolean = Value{true};
        const auto integer = Value{int64_t{42}};
        const auto nativeInteger = Value{42};
        const auto floatingPoint = Value{2.5};
        const auto text = Value{"hello"_el};

        REQUIRE(nullValue.isNull());
        REQUIRE_FALSE(nullValue.isTruthy());
        REQUIRE(boolean.isBoolean());
        REQUIRE(boolean.asBoolean());
        REQUIRE(integer.isInteger());
        REQUIRE_EQUAL(integer.asInteger(), int64_t{42});
        REQUIRE_EQUAL(nativeInteger.asInteger(), int64_t{42});
        REQUIRE(floatingPoint.isFloat());
        REQUIRE_EQUAL(floatingPoint.asFloat(), 2.5);
        REQUIRE(text.isText());
        REQUIRE_EQUAL(text.asText(), "hello"_el);
        REQUIRE_EQUAL(nullValue.toString(), ""_el);
        REQUIRE_EQUAL(boolean.toString(), "true"_el);
        REQUIRE_EQUAL(integer.toString(), "42"_el);
        REQUIRE_EQUAL(floatingPoint.toString(), "2.5"_el);
        REQUIRE_EQUAL(text.toString(), "hello"_el);
        REQUIRE_THROWS_AS(el::err::LogicError, text.asInteger());
    }

    void testCollectionsAndLookup() {
        const auto list = Value{ValueList{Value{"first"_el}, Value{int64_t{2}}}};
        auto values = ValueMap{};
        values.set("name"_el, "Ada"_el);
        const auto map = Value{values};

        REQUIRE(list.isList());
        REQUIRE_EQUAL(list.itemCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(list.asList().count(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(list.get(el::unit::ItemIndex{0U}).asText(), "first"_el);
        REQUIRE(list.get(el::unit::ItemIndex{9U}).isNull());
        REQUIRE(map.isMap());
        REQUIRE_EQUAL(map.itemCount(), el::unit::ItemCount{1U});
        REQUIRE_EQUAL(map.asMap().count(), el::unit::ItemCount{1U});
        REQUIRE_EQUAL(map.get("name"_el).asText(), "Ada"_el);
        REQUIRE(map.get("missing"_el).isNull());
        REQUIRE_THROWS_AS(el::err::LogicError, list.asMap());
        REQUIRE_THROWS_AS(el::err::LogicError, map.asList());
    }

    void testTruthAndCallback() {
        REQUIRE_FALSE(Value{false}.isTruthy());
        REQUIRE_FALSE(Value{int64_t{0}}.isTruthy());
        REQUIRE_FALSE(Value{0.0}.isTruthy());
        REQUIRE_FALSE(Value{""_el}.isTruthy());
        REQUIRE(Value{"x"_el}.isTruthy());
        REQUIRE_FALSE(Value{ValueList{}}.isTruthy());
        REQUIRE(Value{ValueCallbackFn{[]() -> Value { return "late"_el; }}}.isTruthy());
        REQUIRE_EQUAL(Value{ValueCallbackFn{[]() -> Value { return "late"_el; }}}.evaluate().asText(), "late"_el);
    }
};
