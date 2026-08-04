// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/EscapeAmount.hpp>
#include <erbsland/text/json/JsonValue.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>

using namespace el::text::literals;
using namespace el::text::json;

TESTED_TARGETS(JsonValue JsonParseOptions JsonFormatOptions)
class JsonParserTest final : public el::UnitTest {
public:
    void testStrictParsingAndSerialization() {
        const auto value = JsonValue::fromStringOrThrow(R"({"z":[null,true,-12,1.5e2],"a":"\uD83E\uDD8A\n"})"_el);
        REQUIRE(value.is(JsonType::Object));
        REQUIRE_EQUAL(value.getOrThrow("z"_el).itemCount(), el::unit::ItemCount{4U});
        REQUIRE_EQUAL(value.getOrThrow("a"_el).getTextOrThrow(), u8"🦊\n"_el);
        REQUIRE_EQUAL(value.toString(), R"({"a":"🦊\n","z":[null,true,-12,150]})"_el);

        const auto pretty = value.toString(JsonFormatOptions::pretty());
        REQUIRE(pretty.contains("\n  \"a\": "_el));
        const auto reparsed = JsonValue::fromStringOrThrow(pretty);
        REQUIRE_EQUAL(reparsed.toString(), value.toString());

        REQUIRE_EQUAL(
            JsonValue::fromStringOrThrow("9223372036854775807"_el).getOrThrow<int64_t>(),
            std::numeric_limits<int64_t>::max());
        REQUIRE_EQUAL(
            JsonValue::fromStringOrThrow("-9223372036854775808"_el).getOrThrow<int64_t>(),
            std::numeric_limits<int64_t>::min());
        REQUIRE_FALSE(JsonValue::fromStringOrThrow("9223372036854775808"_el).get<int64_t>().has_value());
        REQUIRE_EQUAL(
            JsonValue{u8"é🦊"_el}.toString(JsonFormatOptions{}.setEscapeAmount(el::text::EscapeAmount::NonAscii)),
            "\"\\u00E9\\uD83E\\uDD8A\""_el);
    }

    void testMalformedDocuments() {
        REQUIRE_FALSE(JsonValue::fromString(""_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("[1,]"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("{\"a\":1,\"a\":2}"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("01"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("1."_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("\"\\uD800\""_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("\"\\uDC00\""_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("\"line\nfeed\""_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("{\"a\":1,\"\\u0061\":2}"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("1e309"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("+1"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString(".1"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("[1 2]"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("true\v"_el).has_value());
        REQUIRE_FALSE(JsonValue::fromString("true false"_el).has_value());
        REQUIRE_THROWS_AS(el::err::ParseError, JsonValue::fromStringOrThrow("["_el));
    }

    void testLimits() {
        const auto inputLimit = JsonParseOptions{}.setMaximumInputLength(el::unit::ByteLength{3U});
        REQUIRE_FALSE(JsonValue::fromString("null"_el, inputLimit).has_value());
        const auto nestingLimit = JsonParseOptions{}.setMaximumNesting(el::unit::ItemCount{1U});
        REQUIRE(JsonValue::fromString("[1]"_el, nestingLimit).has_value());
        REQUIRE_FALSE(JsonValue::fromString("[[1]]"_el, nestingLimit).has_value());
        const auto valueLimit = JsonParseOptions{}.setMaximumValueCount(el::unit::ItemCount{2U});
        REQUIRE(JsonValue::fromString("[1]"_el, valueLimit).has_value());
        REQUIRE_FALSE(JsonValue::fromString("[1,2]"_el, valueLimit).has_value());
        const auto stringLimit = JsonParseOptions{}.setMaximumStringLength(el::unit::CpLength{2U});
        REQUIRE_FALSE(JsonValue::fromString("\"abc\""_el, stringLimit).has_value());
        REQUIRE_FALSE(JsonValue::fromString("{\"abc\":0}"_el, stringLimit).has_value());

        REQUIRE_EQUAL(JsonParseOptions::cDefaultMaximumInputLength, el::unit::ByteLength{16U * 1024U * 1024U});
        REQUIRE_EQUAL(JsonParseOptions::cDefaultMaximumNesting, el::unit::ItemCount{64U});
        REQUIRE_EQUAL(JsonParseOptions::cDefaultMaximumValueCount, el::unit::ItemCount{1'000'000U});
        REQUIRE_EQUAL(JsonParseOptions::cDefaultMaximumStringLength, el::unit::CpLength{8U * 1024U * 1024U});
    }
};
