// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/http/HttpFieldType.hpp>
#include <erbsland/network/http_server/HttpCookieSessionManager.hpp>
#include <erbsland/network/http_server/HttpServerRequest.hpp>
#include <erbsland/network/http_server/HttpServerSessionContext.hpp>
#include <erbsland/network/impl/http/server/HttpServerSession.hpp>
#include <erbsland/text/AnyString.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringCharReader.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <memory>
#include <thread>
#include <utility>

using namespace el::event;
using namespace el::network;
using namespace el::text;
using namespace el::text::literals;

TESTED_TARGETS(HttpCookieSessionManager HttpCookieSessionManagerOptions HttpServerSession)
class HttpCookieSessionManagerTest final : public el::UnitTest {
    class MockRequest final : public HttpServerRequest {
    public:
        MockRequest(EventsPtr owner, HttpHeaders headers) :
            HttpServerRequest{std::move(owner)},
            _head{HttpMethodType::Get, "/"_el, HttpVersion::Http11, std::move(headers)} {}

        [[nodiscard]] auto head() const noexcept -> const HttpRequestHead & override { return _head; }
        [[nodiscard]] auto path() const noexcept -> const String & override { return _path; }
        [[nodiscard]] auto query() const noexcept -> const String & override { return _query; }
        [[nodiscard]] auto parameter(const String &) const -> std::optional<String> override { return {}; }
        [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override { return {}; }
        [[nodiscard]] auto remoteEndpoint() const -> std::optional<IpEndpoint> override { return {}; }
        [[nodiscard]] auto connection() const noexcept -> const ConnectionPtr & override { return _connection; }
        [[nodiscard]] auto session() const -> HttpServerSessionPtr override { return {}; }
        void streamBody() override {}
        void aggregateBody(el::unit::ByteLength) override {}
        void rejectBody() override {}
        void pauseBody() override {}
        void resumeBody() override {}
        void sendResponse(HttpResponseHead, el::mem::ByteBlock) override {}
        void sendText(String, HttpStatus, HttpHeaders) override {}
        void sendJson(String, HttpStatus, HttpHeaders) override {}
        void sendJson(const el::text::json::JsonValue &, HttpStatus, HttpHeaders) override {}
        void sendError(HttpStatus, String) override {}
        void sendRedirect(String, HttpStatus, HttpHeaders) override {}
        void startResponse(HttpResponseHead) override {}
        [[nodiscard]] auto sendBody(const el::mem::ByteBlock &) -> NetworkSendStatus override {
            return NetworkSendStatus::Closed;
        }
        void finishBody(HttpHeaders) override {}
        [[nodiscard]] auto isResponseStarted() const noexcept -> bool override { return false; }
        [[nodiscard]] auto isFinal() const noexcept -> bool override { return false; }
        [[nodiscard]] auto events() -> HttpServerRequestEventEditor & override {
            throw el::err::LogicError{"Mock request events are unavailable."_el};
        }

    private:
        HttpRequestHead _head;
        String _path{"/"};
        String _query;
        ConnectionPtr _connection;
    };

    struct Harness final {
        ApplicationTestScope<> applicationScope;
        EventLoopPtr loop{EventLoop::create()};
        HttpCookieSessionManagerPtr manager;
        std::size_t created{};

        explicit Harness(HttpCookieSessionManagerOptions options = {}) :
            manager{HttpCookieSessionManager::create(std::move(options))} {}

        [[nodiscard]] auto select(const String &cookie = {}, const bool secure = false) -> HttpServerSessionSelection {
            auto result = HttpServerSessionSelection{};
            auto completed = false;
            loop->invoke([&]() -> void {
                auto headers = HttpHeaders{};
                if (!cookie.isEmpty()) {
                    headers.addField(HttpFieldType::Cookie, cookie);
                }
                const auto request = std::make_shared<MockRequest>(loop, std::move(headers));
                auto context = HttpServerSessionContext{
                    request,
                    secure,
                    [&](std::optional<String> identifier, HttpSessionDataPtr data) -> HttpServerSessionPtr {
                        ++created;
                        return std::make_shared<el::network::impl::HttpServerSession>(
                            loop, std::move(identifier), std::move(data), [](const HttpServerSessionPtr &) {});
                    }};
                result = manager->selectSession(context);
                completed = true;
            });
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{1};
            while (!completed && std::chrono::steady_clock::now() < deadline) {
                static_cast<void>(loop->runOnce(el::time::TimeDelta::milliseconds(25)));
            }
            if (loop->hasError()) {
                std::rethrow_exception(loop->takeError());
            }
            if (!completed) {
                throw el::err::LogicError{"The cookie-manager test callback was not invoked."_el};
            }
            return result;
        }
    };

private:
    [[nodiscard]] static auto tokenFromSetCookie(const String &value) -> String {
        auto reader = StringCharReader{value};
        reader.advanceUntil(CharSet{U'='});
        if (!reader.advanceIf(U'=')) {
            throw el::err::LogicError{"The cookie-manager test received an invalid Set-Cookie field."_el};
        }
        reader.startCapture();
        reader.advanceUntil(CharSet{U';'});
        return reader.takeCapture().toString();
    }

    [[nodiscard]] static auto requestCookie(const String &token) -> String {
        auto editor = StringEditor{};
        editor.append("erbsland-session="_el);
        editor.append(token);
        return String{editor};
    }

public:
    void testCreationAttributesAndReuse() {
        auto harness = Harness{};
        const auto first = harness.select();
        REQUIRE_EQUAL(harness.created, 1U);
        REQUIRE(first.isValid());
        REQUIRE(first.session()->identifier().has_value());
        REQUIRE_EQUAL(first.session()->identifier()->length(), el::unit::ByteLength{43U});
        REQUIRE(first.reservedCookieName().has_value());
        REQUIRE_EQUAL(*first.reservedCookieName(), "erbsland-session"_el);

        const auto setCookie = first.responseFields().getFirst(HttpFieldType::SetCookie);
        REQUIRE(setCookie.contains("; Path=/"_el));
        REQUIRE(setCookie.contains("; HttpOnly"_el));
        REQUIRE(setCookie.contains("; SameSite=Lax"_el));
        REQUIRE_FALSE(setCookie.contains("; Secure"_el));
        REQUIRE(setCookie.contains("; Max-Age=86400"_el));

        const auto token = tokenFromSetCookie(setCookie);
        const auto second = harness.select(requestCookie(token));
        REQUIRE_EQUAL(second.session(), first.session());
        REQUIRE(second.responseFields().fieldCount().isZero());
        REQUIRE_EQUAL(harness.created, 1U);
    }

    void testSecureUnknownMalformedAndDuplicateCookies() {
        auto harness = Harness{};
        const auto first = harness.select({}, true);
        const auto setCookie = first.responseFields().getFirst(HttpFieldType::SetCookie);
        REQUIRE(setCookie.contains("; Secure"_el));
        const auto token = tokenFromSetCookie(setCookie);

        const auto unknown = harness.select("erbsland-session=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"_el);
        REQUIRE_NOT_EQUAL(unknown.session(), first.session());

        const auto malformed = harness.select("erbsland-session=bad"_el);
        REQUIRE_NOT_EQUAL(malformed.session(), first.session());

        auto duplicate = StringEditor{};
        duplicate.append(requestCookie(token));
        duplicate.append("; "_el);
        duplicate.append(requestCookie(token));
        const auto duplicateSelection = harness.select(String{duplicate});
        REQUIRE_NOT_EQUAL(duplicateSelection.session(), first.session());
    }

    void testBoundedIdleEviction() {
        auto options = HttpCookieSessionManagerOptions{};
        options.setMaximumSessions(el::unit::ItemCount{2U});
        auto harness = Harness{options};
        const auto first = harness.select();
        const auto firstCookie =
            requestCookie(tokenFromSetCookie(first.responseFields().getFirst(HttpFieldType::SetCookie)));
        const auto second = harness.select();
        const auto secondCookie =
            requestCookie(tokenFromSetCookie(second.responseFields().getFirst(HttpFieldType::SetCookie)));
        REQUIRE_EQUAL(harness.select(firstCookie).session(), first.session());
        (void)harness.select();
        REQUIRE_EQUAL(harness.select(firstCookie).session(), first.session());
        REQUIRE_NOT_EQUAL(harness.select(secondCookie).session(), second.session());
    }

    void testIdleExpiryAndDeletionCookie() {
        auto options = HttpCookieSessionManagerOptions{};
        options.setIdleTimeout(el::time::TimeDelta::milliseconds(2));
        auto harness = Harness{options};
        const auto first = harness.select();
        const auto cookie =
            requestCookie(tokenFromSetCookie(first.responseFields().getFirst(HttpFieldType::SetCookie)));
        std::this_thread::sleep_for(std::chrono::milliseconds{4});
        REQUIRE_NOT_EQUAL(harness.select(cookie).session(), first.session());

        auto deletion = HttpHeaders{};
        auto invalidated = false;
        harness.loop->invoke([&]() -> void {
            first.session()->invalidate();
            deletion = harness.manager->sessionInvalidated(first.session());
            invalidated = true;
        });
        while (!invalidated) {
            REQUIRE(harness.loop->runOnce(el::time::TimeDelta::milliseconds(25)));
        }
        const auto value = deletion.getFirst(HttpFieldType::SetCookie);
        REQUIRE(value.startsWith("erbsland-session="_el));
        REQUIRE(value.contains("; Max-Age=0"_el));
    }

    void testOptionValidation() {
        auto options = HttpCookieSessionManagerOptions{};
        options.setCookieName("bad name"_el);
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpCookieSessionManager::create(options));
        options = HttpCookieSessionManagerOptions{};
        options.setSameSite(HttpCookieSameSite::None).setSecurePolicy(HttpCookieSecurePolicy::Never);
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpCookieSessionManager::create(options));
    }
};
