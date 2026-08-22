// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/network/http/HttpRequestHead.hpp>
#include <erbsland/network/http/HttpResponseHead.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpRequestHead HttpResponseHead)
class HttpMessageTest final : public el::UnitTest {
public:
    void testRequestHead() {
        REQUIRE_FALSE(HttpRequestHead{}.isValid());
        auto headers = HttpHeaders{};
        headers.addField(HttpFieldType::Host, "example.test"_el);
        const auto head = HttpRequestHead{HttpMethodType::Get, "/exact?x=%2f"_el, HttpVersion::Http11, headers};
        REQUIRE(head.isValid());
        REQUIRE_EQUAL(head.method(), HttpMethod{HttpMethodType::Get});
        REQUIRE_EQUAL(head.target(), "/exact?x=%2f"_el);
        REQUIRE_EQUAL(head.version(), HttpVersion::Http11);
        REQUIRE_EQUAL(head.headers(), headers);

        REQUIRE_THROWS_AS(
            el::err::ParameterError, HttpRequestHead(HttpMethodType::Get, "contains space"_el, HttpVersion::Http11));
        REQUIRE_THROWS_AS(
            el::err::ParameterError, HttpRequestHead(HttpMethodType::Get, u8"/non-ascii-é"_el, HttpVersion::Http11));
    }

    void testResponseHeadAndRawReason() {
        REQUIRE_FALSE(HttpResponseHead{}.isValid());
        const auto malformed = el::text::String{el::unittest::th::stdStringFromHex("52 61 77 FF")};
        const auto head = HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, malformed};
        REQUIRE(head.isValid());
        REQUIRE_EQUAL(head.status(), HttpStatus::Ok);
        REQUIRE_EQUAL(head.reasonPhrase().length(), el::unit::ByteLength{4U});
        REQUIRE_FALSE(head.reasonPhrase().isValidUtf8());

        const auto control = el::text::String{el::unittest::th::stdStringFromHex("42 61 64 0A")};
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpResponseHead(HttpVersion::Http11, HttpStatus::Ok, control));
    }

    void testFieldEdgeWhitespacePolicy() {
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpField("X-Test"_el, " leading"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpField("X-Test"_el, "trailing\t"_el));
        REQUIRE_NOTHROW(HttpField("X-Test"_el, "interior\tvalue"_el));
    }
};
