// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/http_server/HttpServerSession.hpp>
#include <erbsland/network/impl/http/server/HttpRequestTarget.hpp>
#include <erbsland/network/impl/http/server/HttpServerSession.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpServerSession HttpServerSessionEventEditor HttpSessionData HttpServerRequestHeadFn)
class HttpServerSessionTest final : public el::UnitTest {
    class Data final : public HttpSessionData {
    public:
        int value{};
    };

public:
    void testDataOrderedRoutesAndInvalidation() {
        const auto loop = EventLoop::create();
        auto session = std::shared_ptr<el::network::impl::HttpServerSession>{};
        auto invalidationObserverCount = 0;
        auto invalidatedCount = 0;
        auto finalCount = 0;
        auto selectedRoute = 0;
        loop->invoke([&]() -> void {
            session = std::make_shared<el::network::impl::HttpServerSession>(
                loop, "identified"_el, std::make_shared<Data>(), [&](const HttpServerSessionPtr &) -> void {
                    ++invalidationObserverCount;
                });
            auto replacementData = std::make_shared<Data>();
            replacementData->value = 42;
            session->setData(replacementData);
            session->events()
                .onRequestHead(
                    HttpMethod{HttpMethodType::Get},
                    "/value/{id}"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr) -> void { selectedRoute = 1; })
                .onRequestHead(
                    HttpMethod{HttpMethodType::Get},
                    "/value/special"_el,
                    [&](HttpServerSessionPtr, HttpServerRequestPtr) -> void { selectedRoute = 2; })
                .onInvalidated([&]() -> void { ++invalidatedCount; })
                .onFinal([&]() -> void { ++finalCount; });

            const auto target = el::network::impl::HttpRequestTarget{"/value/special"_el};
            const auto match = session->routes().match(HttpMethod{HttpMethodType::Get}, target.segments());
            REQUIRE(match.handler.isValid());
            REQUIRE_EQUAL(match.parameters.size(), 1U);
            std::get<HttpServerRequestHeadFn>(match.handler.callback())(session, {});
            REQUIRE_EQUAL(selectedRoute, 1);
            REQUIRE_EQUAL(session->identifier(), std::optional<el::text::String>{"identified"_el});
            REQUIRE_EQUAL(std::dynamic_pointer_cast<Data>(session->data())->value, 42);

            session->invalidate();
            session->invalidate();
        });
        REQUIRE(loop->runOnce());
        REQUIRE(session);
        REQUIRE_FALSE(session->isValid());
        REQUIRE_EQUAL(invalidationObserverCount, 1);
        REQUIRE_EQUAL(invalidatedCount, 1);
        REQUIRE_EQUAL(finalCount, 1);
    }
};
