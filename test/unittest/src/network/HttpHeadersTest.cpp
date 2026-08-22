// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/network/http/HttpHeaders.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpFieldType HttpFieldName HttpField HttpHeaderLimits HttpHeaders)
class HttpHeadersTest final : public el::UnitTest {
public:
    void testNamesAndFields() {
        const auto original = HttpFieldName{"x-Custom-Name"_el};
        const auto folded = HttpFieldName{"X-CUSTOM-NAME"_el};
        REQUIRE_EQUAL(original, folded);
        REQUIRE_EQUAL(original.toString(), "x-Custom-Name"_el);
        REQUIRE_EQUAL(original.toHash(), folded.toHash());
        REQUIRE_EQUAL(HttpFieldName{HttpFieldType::ContentType}.toString(), "Content-Type"_el);
        REQUIRE_EQUAL(HttpFieldName{"content-type"_el}.type(), HttpFieldType::ContentType);
        REQUIRE_EQUAL(HttpFieldName{"pragma"_el}.type(), HttpFieldType::Pragma);
        REQUIRE(HttpFieldType{HttpFieldType::Host}.isRequestField());
        REQUIRE_FALSE(HttpFieldType{HttpFieldType::Host}.isResponseField());
        REQUIRE_FALSE(HttpFieldType{HttpFieldType::Allow}.isRequestField());
        REQUIRE(HttpFieldType{HttpFieldType::Allow}.isResponseField());
        REQUIRE(HttpFieldType{HttpFieldType::SetCookie}.isResponseField());
        REQUIRE_FALSE(HttpFieldType{HttpFieldType::SetCookie}.isRequestField());

        REQUIRE_THROWS_AS(el::err::ParameterError, HttpFieldName{"bad name"_el});
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpField{"Good"_el, "bad\r\nvalue"_el});
        REQUIRE_NOTHROW(HttpField{"Good"_el, "a\tb"_el});

        for (const auto *hex : {"00", "01", "7F"}) {
            const auto control = el::text::String{el::unittest::th::stdStringFromHex(hex)};
            REQUIRE_THROWS_AS(el::err::ParameterError, HttpField("Good"_el, control));
        }

        const auto malformedBytes = el::unittest::th::stdStringFromHex("6F 6B FF");
        const auto malformed = el::text::String{malformedBytes};
        const auto field = HttpField{"X-Raw"_el, malformed};
        REQUIRE_EQUAL(field.value().length(), el::unit::ByteLength{3U});
        REQUIRE_FALSE(field.value().isValidUtf8());
    }

    void testOrderingLookupAndCopyOnWrite() {
        auto headers = HttpHeaders{};
        headers.addField("X-First"_el, "one"_el)
            .addField(HttpFieldType::SetCookie, "a=1"_el)
            .addField("x-first"_el, "two"_el)
            .addField(HttpFieldType::SetCookie, "b=2"_el);
        REQUIRE_EQUAL(headers.fieldCount(), el::unit::ItemCount{4U});
        REQUIRE(headers.hasField("X-FIRST"_el));
        REQUIRE_EQUAL(headers.getFirst("x-first"_el), "one"_el);
        REQUIRE_EQUAL(headers.getAll("X-First"_el).count(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(headers.getAll(HttpFieldType::SetCookie).get(el::unit::ItemIndex{1U}), "b=2"_el);

        auto copy = headers;
        copy.setField("X-FIRST"_el, "replacement"_el);
        REQUIRE_EQUAL(copy.fieldCount(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(copy.field(el::unit::ItemIndex::zero()).value(), "replacement"_el);
        REQUIRE_EQUAL(copy.field(el::unit::ItemIndex{1U}).name().type(), HttpFieldType::SetCookie);
        REQUIRE_EQUAL(headers.fieldCount(), el::unit::ItemCount{4U});
        REQUIRE_EQUAL(headers.getFirst("x-first"_el), "one"_el);

        REQUIRE_EQUAL(copy.removeAllFields(HttpFieldType::SetCookie), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(copy.fieldCount(), el::unit::ItemCount::one());
    }

    void testContentType() {
        auto headers = HttpHeaders{};
        headers.setContentType(HttpMediaType{"application"_el, "json"_el}.setParameter("charset"_el, "utf-8"_el));
        REQUIRE(headers.contentType().has_value());
        REQUIRE_EQUAL(headers.contentType()->subtype(), "json"_el);
        REQUIRE_EQUAL(headers.getFirst(HttpFieldType::ContentType), "application/json; charset=utf-8"_el);
        headers.setField(HttpFieldType::ContentType, "invalid"_el);
        REQUIRE_FALSE(headers.contentType().has_value());
    }

    void testLimitsAndStrongFailures() {
        const auto countLimits = HttpHeaderLimits{}.setMaximumFieldCount(el::unit::ItemCount::one());
        auto countHeaders = HttpHeaders{countLimits};
        countHeaders.addField("A"_el, "1"_el);
        REQUIRE_THROWS_AS(el::err::ParameterError, countHeaders.addField("B"_el, "2"_el));
        REQUIRE_EQUAL(countHeaders.fieldCount(), el::unit::ItemCount::one());

        auto nameHeaders = HttpHeaders{HttpHeaderLimits{}.setMaximumNameLength(el::unit::ByteLength{3U})};
        REQUIRE_THROWS_AS(el::err::ParameterError, nameHeaders.addField("Long"_el, "x"_el));
        REQUIRE(nameHeaders.fieldCount().isZero());

        auto valueHeaders = HttpHeaders{HttpHeaderLimits{}.setMaximumValueLength(el::unit::ByteLength{2U})};
        REQUIRE_THROWS_AS(el::err::ParameterError, valueHeaders.addField("A"_el, "123"_el));
        REQUIRE(valueHeaders.fieldCount().isZero());

        auto aggregateHeaders = HttpHeaders{HttpHeaderLimits{}.setMaximumAggregateLength(el::unit::ByteLength{7U})};
        aggregateHeaders.addField("A"_el, "12"_el);
        REQUIRE_EQUAL(aggregateHeaders.serializedLength(), el::unit::ByteLength{7U});
        REQUIRE_THROWS_AS(el::err::ParameterError, aggregateHeaders.addField("B"_el, ""_el));
        REQUIRE_EQUAL(aggregateHeaders.fieldCount(), el::unit::ItemCount::one());
    }
};
