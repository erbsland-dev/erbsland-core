// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/json/JsonValue.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>

using namespace el::text::literals;
using namespace el::text::json;

TESTED_TARGETS(JsonValue JsonType)
class JsonValueTest final : public el::UnitTest {
public:
    void testPrimitiveValuesAndConversions() {
        REQUIRE(JsonValue{}.is(JsonType::Null));
        REQUIRE(JsonValue{}.isPrimitive());
        REQUIRE_EQUAL(JsonValue{}.itemCount(), el::unit::ItemCount::zero());
        REQUIRE(JsonValue{true}.getBoolOrThrow());
        REQUIRE_FALSE(JsonValue{false}.getBool(true));
        REQUIRE_EQUAL(JsonValue{int64_t{42}}.getOrThrow<int64_t>(), int64_t{42});
        REQUIRE_EQUAL(JsonValue{42.0}.getOrThrow<int64_t>(), int64_t{42});
        REQUIRE_FALSE(JsonValue{42.5}.get<int64_t>().has_value());
        REQUIRE_EQUAL(JsonValue{int64_t{42}}.getNumberOrThrow(), 42.0);
        REQUIRE_EQUAL(JsonValue{"text"_el}.getTextOrThrow(), "text"_el);
        REQUIRE_EQUAL(JsonValue{}.get<int64_t>(17), int64_t{17});
        REQUIRE_EQUAL(JsonValue{}.getText("fallback"_el), "fallback"_el);
        REQUIRE(JsonValue{JsonArray{}}.is(JsonType::Array));
        REQUIRE(JsonValue{JsonObject{}}.is(JsonType::Object));
        REQUIRE_THROWS_AS(el::err::ParameterError, JsonValue(std::numeric_limits<double>::infinity()));
        REQUIRE_THROWS_AS(el::err::ParameterError, JsonValue(std::numeric_limits<double>::quiet_NaN()));
        REQUIRE_THROWS_AS(el::err::ParameterError, JsonValue(std::numeric_limits<uint64_t>::max()));
        REQUIRE_THROWS_AS(el::err::LogicError, JsonValue{}.getBoolOrThrow());
    }

    void testArrayAndObjectMutation() {
        auto array = JsonValue{JsonArray{JsonValue{1}}};
        array.append(JsonValue{2}).set(el::unit::ItemIndex::zero(), JsonValue{3});
        REQUIRE_EQUAL(array.itemCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(array.getOrThrow(el::unit::ItemIndex::zero()).getOrThrow<int64_t>(), int64_t{3});
        REQUIRE(array.get(el::unit::ItemIndex{9U}).is(JsonType::Null));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, array.set(el::unit::ItemIndex{3U}, JsonValue{}));
        REQUIRE_THROWS_AS(el::err::LogicError, JsonValue{}.append(JsonValue{}));

        auto object = JsonValue{JsonObject{}};
        object.set("key"_el, JsonValue{7});
        REQUIRE_EQUAL(object.getOrThrow("key"_el).getOrThrow<int64_t>(), int64_t{7});
        object.set("key"_el, JsonValue{8});
        REQUIRE_EQUAL(object.getOrThrow("key"_el).getOrThrow<int64_t>(), int64_t{8});
        REQUIRE(object.get("missing"_el).is(JsonType::Null));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, object.getOrThrow("missing"_el));
        REQUIRE_THROWS_AS(el::err::LogicError, JsonValue{}.set("key"_el, JsonValue{}));
    }

    void testCopyOnWrite() {
        auto original = JsonValue{JsonArray{JsonValue{1}}};
        auto copy = original;
        original.set(el::unit::ItemIndex::zero(), JsonValue{9});
        REQUIRE_EQUAL(original.getOrThrow(el::unit::ItemIndex::zero()).getOrThrow<int64_t>(), int64_t{9});
        REQUIRE_EQUAL(copy.getOrThrow(el::unit::ItemIndex::zero()).getOrThrow<int64_t>(), int64_t{1});

        auto nestedObject = JsonObject{};
        nestedObject.set("items"_el, JsonValue{JsonArray{JsonValue{1}}});
        auto nested = JsonValue{std::move(nestedObject)};
        auto nestedCopy = nested;
        auto items = nested.getOrThrow("items"_el);
        items.append(JsonValue{2});
        nested.set("items"_el, std::move(items));
        REQUIRE_EQUAL(nested.getOrThrow("items"_el).itemCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(nestedCopy.getOrThrow("items"_el).itemCount(), el::unit::ItemCount{1U});
    }
};
