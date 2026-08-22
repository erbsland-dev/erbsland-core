// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/network/impl/http/server/HttpRequestTarget.hpp>
#include <erbsland/network/impl/http/server/HttpRoutes.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

using HttpRequestTarget = el::network::impl::HttpRequestTarget;
using HttpRouteHandler = el::network::impl::HttpRouteHandler;
using HttpRoutes = el::network::impl::HttpRoutes;

TESTED_TARGETS(HttpRoutes HttpRequestTarget HttpServerRouteOptions)
class HttpRoutesTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto handler() -> HttpRouteHandler {
        return HttpRouteHandler{HttpServerRequestHeadFn{[](HttpServerSessionPtr, HttpServerRequestPtr) {}}};
    }

public:
    void testRequestTargetDecodingAndQuerySeparation() {
        const auto target = HttpRequestTarget{"/caf%C3%A9/a%2Fb?raw=%2F+value"_el};
        REQUIRE_EQUAL(target.path(), "/café/a/b"_el);
        REQUIRE_EQUAL(target.query(), "raw=%2F+value"_el);
        REQUIRE_EQUAL(target.segments().size(), 2U);
        REQUIRE_EQUAL(target.segments()[0], "café"_el);
        REQUIRE_EQUAL(target.segments()[1], "a/b"_el);

        const auto normalized = HttpRequestTarget{"/cafe%CC%81"_el};
        REQUIRE_EQUAL(normalized.path(), "/café"_el);
        REQUIRE_EQUAL(normalized.segments()[0], "café"_el);
    }

    void testMalformedRequestTargets() {
        REQUIRE_THROWS_AS(el::err::ParseError, HttpRequestTarget{"relative"_el});
        REQUIRE_THROWS_AS(el::err::ParseError, HttpRequestTarget{"/fragment#bad"_el});
        REQUIRE_THROWS_AS(el::err::ParseError, HttpRequestTarget{"/bad%"_el});
        REQUIRE_THROWS_AS(el::err::ParseError, HttpRequestTarget{"/bad%GG"_el});
        REQUIRE_THROWS_AS(el::err::ParseError, HttpRequestTarget{"/bad%C3%28"_el});
    }

    void testRegistrationOrderAndParameters() {
        auto routes = HttpRoutes{};
        const auto first = handler();
        const auto second = handler();
        routes.add(HttpMethod{HttpMethodType::Get}, "/users/{id}"_el, first);
        routes.add(HttpMethod{HttpMethodType::Get}, "/users/special"_el, second);

        const auto target = HttpRequestTarget{"/users/special?ignored=yes"_el};
        const auto match = routes.match(HttpMethod{HttpMethodType::Get}, target.segments());
        REQUIRE(match.handler.isValid());
        REQUIRE_EQUAL(match.parameters.size(), 1U);
        REQUIRE_EQUAL(match.parameters[0].first, "id"_el);
        REQUIRE_EQUAL(match.parameters[0].second, "special"_el);
    }

    void testCatchAllAndEncodedSlashStayWithinSegments() {
        auto routes = HttpRoutes{};
        routes.add(HttpMethod{HttpMethodType::Get}, "/files/{*path}"_el, handler());

        const auto match =
            routes.match(HttpMethod{HttpMethodType::Get}, HttpRequestTarget{"/files/a%2Fb/c"_el}.segments());
        REQUIRE(match.handler.isValid());
        REQUIRE_EQUAL(match.parameters.size(), 1U);
        REQUIRE_EQUAL(match.parameters[0].first, "path"_el);
        REQUIRE_EQUAL(match.parameters[0].second, "a/b/c"_el);
    }

    void testMethodFilteringAllowAndHeadFallback() {
        auto routes = HttpRoutes{};
        const auto getHandler = handler();
        const auto headHandler = handler();
        routes.add(HttpMethod{HttpMethodType::Get}, "/value"_el, getHandler);
        routes.add(HttpMethod{HttpMethodType::Post}, "/value"_el, handler());

        const auto put = routes.match(HttpMethod{HttpMethodType::Put}, HttpRequestTarget{"/value"_el}.segments());
        REQUIRE_FALSE(put.handler.isValid());
        REQUIRE(put.pathMatched);
        REQUIRE_EQUAL(put.allowed.size(), 3U);
        REQUIRE_EQUAL(put.allowed[0], HttpMethod{HttpMethodType::Get});
        REQUIRE_EQUAL(put.allowed[1], HttpMethod{HttpMethodType::Head});
        REQUIRE_EQUAL(put.allowed[2], HttpMethod{HttpMethodType::Post});

        const auto fallback = routes.match(HttpMethod{HttpMethodType::Head}, HttpRequestTarget{"/value"_el}.segments());
        REQUIRE(fallback.handler.isValid());
        REQUIRE(fallback.usedHeadFallback);

        routes.add(HttpMethod{HttpMethodType::Head}, "/value"_el, headHandler);
        const auto explicitHead =
            routes.match(HttpMethod{HttpMethodType::Head}, HttpRequestTarget{"/value"_el}.segments());
        REQUIRE(explicitHead.handler.isValid());
        REQUIRE_FALSE(explicitHead.usedHeadFallback);
    }

    void testPatternValidation() {
        auto routes = HttpRoutes{};
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("users/{id}"_el, handler()));
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/users/{id}/{id}"_el, handler()));
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/users/{1id}"_el, handler()));
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/files/{*path}/tail"_el, handler()));
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/files/{bad-name}"_el, handler()));
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/query?bad"_el, handler()));
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/empty"_el, HttpRouteHandler{}));
    }

    void testAutomaticRouteOptionsValidation() {
        const auto callback =
            HttpServerRequestFn{[](HttpServerSessionPtr, HttpServerRequestPtr, el::mem::ByteBlock) {}};
        auto routes = HttpRoutes{};
        auto options = HttpServerRouteOptions{};
        options.setAcceptedContentTypes({"application/json"_el, "text/*"_el, "*/*"_el, "application/*+json"_el});
        REQUIRE_NOTHROW(routes.add("/valid"_el, HttpRouteHandler{callback, options}));
        options.setAcceptedContentTypes({"application"_el});
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/invalid"_el, HttpRouteHandler{callback, options}));
        options = HttpServerRouteOptions{}.setMaximumBodyLength(el::unit::ByteLength{});
        REQUIRE_THROWS_AS(el::err::ParameterError, routes.add("/zero"_el, HttpRouteHandler{callback, options}));
    }
};
