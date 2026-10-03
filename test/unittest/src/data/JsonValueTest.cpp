// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/json/JsonValue.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/OutOfRangeError.hpp>
#include <erbsland/OutOfRangeFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>

using namespace el::text::literals;
using namespace el::data::json;

TESTED_TARGETS(JsonValue JsonType)
class JsonValueTest final : public el::UnitTest {
private:
    static constexpr auto cEditorField = "editor"_el;
    static constexpr auto cItemsField = "items"_el;
    static constexpr auto cKeyField = "key"_el;
    static constexpr auto cLiteralField = "literal"_el;
    static constexpr auto cMissingField = "missing"_el;

public:
    void testImplicitTextArguments() {
        JsonValue literal = cLiteralField;
        JsonValue utf8Literal = u8"Grüße"_el;
        auto editor = el::StringEditor{"before"_el};
        JsonValue edited = editor;
        auto object = JsonValue{JsonObject{}};
        object.set(cLiteralField, "value"_el).set(cEditorField, editor);
        auto array = JsonValue{JsonArray{}};
        array.append("item"_el).append(editor);
        editor.append(" after"_el);
        REQUIRE_EQUAL(literal.getTextOrThrow(), cLiteralField);
        REQUIRE_EQUAL(utf8Literal.getTextOrThrow(), "Grüße"_el);
        REQUIRE_EQUAL(edited.getTextOrThrow(), "before"_el);
        REQUIRE_EQUAL(object.getOrThrow(cLiteralField).getTextOrThrow(), "value"_el);
        REQUIRE_EQUAL(object.getOrThrow(cEditorField).getTextOrThrow(), "before"_el);
        REQUIRE_EQUAL(array.getOrThrow(el::ItemIndex{1}).getTextOrThrow(), "before"_el);
    }

    void testLookupExceptionContracts() {
        const auto object = JsonValue{JsonObject{}};
        REQUIRE_THROWS_AS(el::err::NotFoundError, object.getOrThrow(cMissingField));
        auto array = JsonValue{JsonArray{}};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, array.getOrThrow(el::unit::ItemIndex{0U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeFailure, array.set(el::unit::ItemIndex{1U}, JsonValue{}));
        REQUIRE_THROWS_AS(el::err::LogicFailure, array.getOrThrow(cMissingField));
    }

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
        REQUIRE_THROWS_AS(el::err::ParameterFailure, JsonValue(std::numeric_limits<double>::infinity()));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, JsonValue(std::numeric_limits<double>::quiet_NaN()));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, JsonValue(std::numeric_limits<uint64_t>::max()));
        REQUIRE_THROWS_AS(el::err::LogicFailure, JsonValue{}.getBoolOrThrow());
    }

    void testArrayAndObjectMutation() {
        auto array = JsonValue{JsonArray{JsonValue{1}}};
        array.append(JsonValue{2}).set(el::unit::ItemIndex::zero(), JsonValue{3});
        REQUIRE_EQUAL(array.itemCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(array.getOrThrow(el::unit::ItemIndex::zero()).getOrThrow<int64_t>(), int64_t{3});
        REQUIRE(array.get(el::unit::ItemIndex{9U}).is(JsonType::Null));
        REQUIRE_THROWS_AS(el::err::OutOfRangeFailure, array.set(el::unit::ItemIndex{3U}, JsonValue{}));
        REQUIRE_THROWS_AS(el::err::LogicFailure, JsonValue{}.append(JsonValue{}));

        auto object = JsonValue{JsonObject{}};
        object.set(cKeyField, JsonValue{7});
        REQUIRE_EQUAL(object.getOrThrow(cKeyField).getOrThrow<int64_t>(), int64_t{7});
        object.set(cKeyField, JsonValue{8});
        REQUIRE_EQUAL(object.getOrThrow(cKeyField).getOrThrow<int64_t>(), int64_t{8});
        REQUIRE(object.get(cMissingField).is(JsonType::Null));
        REQUIRE_THROWS_AS(el::err::NotFoundError, object.getOrThrow(cMissingField));
        REQUIRE_THROWS_AS(el::err::LogicFailure, JsonValue{}.set(cKeyField, JsonValue{}));
    }

    void testCopyOnWrite() {
        auto original = JsonValue{JsonArray{JsonValue{1}}};
        auto copy = original;
        original.set(el::unit::ItemIndex::zero(), JsonValue{9});
        REQUIRE_EQUAL(original.getOrThrow(el::unit::ItemIndex::zero()).getOrThrow<int64_t>(), int64_t{9});
        REQUIRE_EQUAL(copy.getOrThrow(el::unit::ItemIndex::zero()).getOrThrow<int64_t>(), int64_t{1});

        auto nestedObject = JsonObject{};
        nestedObject.set(cItemsField, JsonValue{JsonArray{JsonValue{1}}});
        auto nested = JsonValue{std::move(nestedObject)};
        auto nestedCopy = nested;
        auto items = nested.getOrThrow(cItemsField);
        items.append(JsonValue{2});
        nested.set(cItemsField, std::move(items));
        REQUIRE_EQUAL(nested.getOrThrow(cItemsField).itemCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(nestedCopy.getOrThrow(cItemsField).itemCount(), el::unit::ItemCount{1U});
    }
};
