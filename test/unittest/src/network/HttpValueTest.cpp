// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/network/http/HttpMediaType.hpp>
#include <erbsland/network/http/HttpMethod.hpp>
#include <erbsland/network/http/HttpStatus.hpp>
#include <erbsland/network/http/HttpVersion.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpMethod HttpMethodType HttpVersion HttpStatus HttpMediaType HttpMediaTypeParameter)
class HttpValueTest final : public el::UnitTest {
public:
    void testMethods() {
        const auto get = HttpMethod{HttpMethodType::Get};
        REQUIRE(get.isValid());
        REQUIRE(get.isStandard());
        REQUIRE_EQUAL(get.toString(), "GET"_el);
        REQUIRE_EQUAL(get.standardType().value(), HttpMethodType::Get);

        const auto extension = HttpMethod::fromStringOrThrow("PURGE"_el);
        REQUIRE(extension.isExtension());
        REQUIRE_FALSE(extension.standardType().has_value());
        REQUIRE_NOT_EQUAL(extension, HttpMethod::fromStringOrThrow("purge"_el));
        REQUIRE(HttpMethodTypes{HttpMethodType::Get}.isSet(HttpMethodType::Get));
        REQUIRE((HttpMethodType::Get | HttpMethodType::Head).contains(HttpMethodTypes{HttpMethodType::Head}));

        REQUIRE_FALSE(HttpMethod{}.isValid());
        REQUIRE_FALSE(HttpMethod{HttpMethodType::All}.isValid());
        REQUIRE_FALSE(HttpMethod::fromString(""_el).isValid());
        REQUIRE_FALSE(HttpMethod::fromString("BAD METHOD"_el).isValid());
        REQUIRE_FALSE(HttpMethod::fromString("GET\r\nX"_el).isValid());
        REQUIRE_THROWS_AS(el::err::ParseError, HttpMethod::fromStringOrThrow(u8"GÉT"_el));
    }

    void testVersions() {
        REQUIRE_FALSE(HttpVersion{}.isValid());
        REQUIRE_EQUAL(HttpVersion{HttpVersion::Http10}.toString(), "HTTP/1.0"_el);
        REQUIRE_EQUAL(HttpVersion{HttpVersion::Http11}.major(), uint8_t{1U});
        REQUIRE_EQUAL(HttpVersion{HttpVersion::Http11}.minor(), uint8_t{1U});
        REQUIRE_EQUAL(HttpVersion::fromStringOrThrow("HTTP/1.1"_el), HttpVersion::Http11);
        REQUIRE_FALSE(HttpVersion::fromString("http/1.1"_el).isValid());
        REQUIRE_FALSE(HttpVersion::fromString("HTTP/2"_el).isValid());
    }

    void testStatuses() {
        REQUIRE_FALSE(HttpStatus{}.isValid());
        REQUIRE(HttpStatus{HttpStatus::Continue}.isInformational());
        REQUIRE(HttpStatus{HttpStatus::Ok}.isSuccessful());
        REQUIRE(HttpStatus{HttpStatus::PermanentRedirect}.isRedirection());
        REQUIRE(HttpStatus{HttpStatus::NotFound}.isClientError());
        REQUIRE(HttpStatus{HttpStatus::ServiceUnavailable}.isServerError());
        REQUIRE_EQUAL(HttpStatus{HttpStatus::Ok}.defaultReasonPhrase(), "OK"_el);
        REQUIRE_EQUAL(HttpStatus{HttpStatus::UseProxy}.defaultReasonPhrase(), "Use Proxy"_el);
        REQUIRE_EQUAL(HttpStatus::fromCode(599U).code(), uint16_t{599U});
        REQUIRE(HttpStatus::fromCode(599U).defaultReasonPhrase().isEmpty());
        REQUIRE_EQUAL(HttpStatus::fromStringOrThrow("431"_el), HttpStatus::RequestHeaderFieldsTooLarge);
        REQUIRE_EQUAL(HttpStatus::fromStringOrThrow("599"_el).toString(), "599"_el);
        REQUIRE_FALSE(HttpStatus::fromCode(99U).isValid());
        REQUIRE_FALSE(HttpStatus::fromCode(600U).isValid());
        REQUIRE_FALSE(HttpStatus::fromString("20"_el).isValid());
        REQUIRE_FALSE(HttpStatus::fromString("+200"_el).isValid());
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpStatus::fromCodeOrThrow(600U));
    }

    void testMediaTypes() {
        const auto parsed = HttpMediaType::fromStringOrThrow(
            "Text/HTML; Charset=\"utf-8\"; title=\"a \\\"quote\\\" and \\\\ slash\""_el);
        REQUIRE_EQUAL(parsed.type(), "text"_el);
        REQUIRE_EQUAL(parsed.subtype(), "html"_el);
        REQUIRE_EQUAL(parsed.parameter("CHARSET"_el).value(), "utf-8"_el);
        REQUIRE_EQUAL(parsed.parameter("title"_el).value(), "a \"quote\" and \\ slash"_el);
        REQUIRE_EQUAL(parsed.toString(), "text/html; charset=utf-8; title=\"a \\\"quote\\\" and \\\\ slash\""_el);

        auto modified = HttpMediaType{"application"_el, "json"_el};
        modified.setParameter("charset"_el, "utf-8"_el).setParameter("profile"_el, "a b"_el);
        modified.setParameter("CHARSET"_el, "UTF-8"_el);
        REQUIRE_EQUAL(modified.parameters().count(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(modified.toString(), "application/json; charset=UTF-8; profile=\"a b\""_el);
        modified.removeParameter("PROFILE"_el);
        REQUIRE_FALSE(modified.hasParameter("profile"_el));

        REQUIRE_FALSE(HttpMediaType::fromString("text"_el).isValid());
        REQUIRE_FALSE(HttpMediaType::fromString("text/"_el).isValid());
        REQUIRE_FALSE(HttpMediaType::fromString("text/plain; charset"_el).isValid());
        REQUIRE_FALSE(HttpMediaType::fromString("text/plain; a=1; A=2"_el).isValid());
        REQUIRE_FALSE(HttpMediaType::fromString("text/plain; a=\"unterminated"_el).isValid());
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpMediaType{"text space"_el, "plain"_el});

        auto oversized = el::text::StringEditor{"text/plain; x="_el};
        oversized.append(U'a', el::unit::CpLength{HttpMediaType::cMaximumTextLength.toRawValue()});
        REQUIRE_FALSE(HttpMediaType::fromString(el::text::String{oversized}).isValid());
    }
};
