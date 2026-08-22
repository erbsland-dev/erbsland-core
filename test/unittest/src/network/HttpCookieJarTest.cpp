// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/http/HttpFieldType.hpp>
#include <erbsland/network/http_client/HttpCookieJar.hpp>
#include <erbsland/network/impl/http/cookie/HttpCookieJar.hpp>
#include <erbsland/network/impl/http/cookie/PublicSuffixList.hpp>
#include <erbsland/network/url/Url.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(HttpCookieJar)
class HttpCookieJarTest final : public el::UnitTest {
public:
    void testManualInsertionMatchingOrderingAndRemoval() {
        const auto loop = EventLoop::create();
        loop->invoke([&]() -> void {
            auto jar = el::network::impl::HttpCookieJar{loop};
            const auto url = Url::fromStringOrThrow("https://www.example.com/a/b/index"_el);
            jar.setCookie(url, "wide"_el, "one"_el);
            auto narrow = HttpCookieInsertionOptions{};
            narrow.setPath("/a/b/index"_el).setSecure(true).setHttpOnly(true).setSameSite(HttpCookieSameSite::Lax);
            jar.setCookie(url, "narrow"_el, "two"_el, narrow);
            REQUIRE_EQUAL(jar.requestCookieHeader(url), "narrow=two; wide=one"_el);
            REQUIRE_EQUAL(jar.cookies().size(), 2U);
            REQUIRE(jar.cookies()[1].isSecure());
            REQUIRE(jar.cookies()[1].isHttpOnly());
            REQUIRE_EQUAL(jar.cookies()[1].sameSite(), HttpCookieSameSite::Lax);
            REQUIRE(jar.removeCookie("wide"_el, "www.example.com"_el, "/a/b"_el));
            REQUIRE_EQUAL(jar.requestCookieHeader(url), "narrow=two"_el);
        });
        static_cast<void>(loop->runOnce());
    }

    void testPeerCookieRulesExpiryPrefixesAndLimits() {
        const auto loop = EventLoop::create();
        loop->invoke([&]() -> void {
            auto jar = el::network::impl::HttpCookieJar{loop};
            auto limits = HttpCookieJarOptions{};
            limits.setMaximumCookies(el::unit::ItemCount{2U})
                .setMaximumCookiesPerRegistrableDomain(el::unit::ItemCount{2U});
            jar.setOptions(limits);
            const auto secure = Url::fromStringOrThrow("https://sub.example.com/account/page"_el);
            REQUIRE_EQUAL(el::network::impl::HttpCookieJar::hostText(secure), "sub.example.com"_el);
            REQUIRE_FALSE(el::network::impl::public_suffix::isPublicSuffix("example.com"_el));
            REQUIRE(jar.storeResponseCookie(secure, "a=1; Domain=example.com; Path=/; Secure; HttpOnly"_el));
            REQUIRE(jar.storeResponseCookie(secure, "gone=x; Max-Age=0; Path=/"_el));
            REQUIRE(jar.storeResponseCookie(secure, "__Host-id=good; Secure; Path=/"_el));
            REQUIRE_FALSE(jar.storeResponseCookie(secure, "__Host-bad=no; Secure; Domain=example.com; Path=/"_el));
            REQUIRE_EQUAL(jar.cookies().size(), 2U);
            REQUIRE_EQUAL(jar.requestCookieHeader(secure), "a=1; __Host-id=good"_el);
            REQUIRE_FALSE(jar.requestCookieHeader(Url::fromStringOrThrow("http://sub.example.com/account"_el))
                    .contains("a=1"_el));
            REQUIRE_EQUAL(jar.cookies().size(), 2U);
            REQUIRE_THROWS(jar.setCookie(
                Url::fromStringOrThrow("https://127.0.0.1/"_el),
                "bad"_el,
                "x"_el,
                HttpCookieInsertionOptions{}.setDomain("0.0.1"_el)));
        });
        static_cast<void>(loop->runOnce());
    }

    void testPeerParsingDefaultPathExpiryAndReplacement() {
        const auto loop = EventLoop::create();
        loop->invoke([&]() -> void {
            auto jar = el::network::impl::HttpCookieJar{loop};
            const auto url = Url::fromStringOrThrow("https://www.example.com/account/profile"_el);
            auto headers = HttpHeaders{};
            headers.addField(HttpFieldType::SetCookie, "token=old; HttpOnly; SameSite=sTrIcT"_el)
                .addField(HttpFieldType::SetCookie, "public=no; Domain=com"_el)
                .addField(HttpFieldType::SetCookie, "expired=no; Expires=Wed, 09 Jun 2021 10:18:14 GMT"_el)
                .addField(
                    HttpFieldType::SetCookie, "precedence=no; Max-Age=0; Expires=Wed, 09 Jun 2099 10:18:14 GMT"_el)
                .addField(HttpFieldType::SetCookie, "fallback=yes; Max-Age=invalid"_el);
            jar.storeResponseCookies(url, headers);
            REQUIRE_EQUAL(jar.cookies().size(), 2U);
            REQUIRE_EQUAL(jar.requestCookieHeader(url), "token=old; fallback=yes"_el);
            REQUIRE(jar.requestCookieHeader(Url::fromStringOrThrow("https://www.example.com/accounting"_el)).isEmpty());
            REQUIRE(jar.storeResponseCookie(url, "token=new; SameSite=Lax"_el));
            REQUIRE_EQUAL(jar.cookies().size(), 2U);
            REQUIRE_EQUAL(jar.cookies()[0].value(), "new"_el);
            REQUIRE_EQUAL(jar.cookies()[0].sameSite(), HttpCookieSameSite::Lax);
        });
        static_cast<void>(loop->runOnce());
    }

    void testIdnaIpValidationAndLruEviction() {
        const auto loop = EventLoop::create();
        loop->invoke([&]() -> void {
            auto jar = el::network::impl::HttpCookieJar{loop};
            auto limits = HttpCookieJarOptions{};
            limits.setMaximumCookies(el::unit::ItemCount{2U})
                .setMaximumCookiesPerRegistrableDomain(el::unit::ItemCount{2U});
            jar.setOptions(limits);
            const auto idnaUrl = Url::fromStringOrThrow("https://bücher.example/a"_el);
            jar.setCookie(idnaUrl, "first"_el, "1"_el);
            jar.setCookie(idnaUrl, "second"_el, "2"_el);
            REQUIRE_EQUAL(jar.cookies()[0].domain(), "xn--bcher-kva.example"_el);
            REQUIRE_EQUAL(jar.requestCookieHeader(idnaUrl), "first=1; second=2"_el);
            jar.setCookie(idnaUrl, "third"_el, "3"_el);
            REQUIRE_EQUAL(jar.cookies().size(), 2U);
            REQUIRE_FALSE(jar.requestCookieHeader(idnaUrl).contains("first=1"_el));

            jar.clear();
            const auto ipUrl = Url::fromStringOrThrow("https://127.0.0.1/a"_el);
            jar.setCookie(ipUrl, "host"_el, "yes"_el);
            REQUIRE_EQUAL(jar.requestCookieHeader(ipUrl), "host=yes"_el);
            REQUIRE(jar.requestCookieHeader(Url::fromStringOrThrow("https://127.0.0.2/a"_el)).isEmpty());
            REQUIRE_THROWS(jar.setCookie(idnaUrl, "bad name"_el, "x"_el));
            REQUIRE_THROWS(
                jar.setCookie(idnaUrl, "__Secure-bad"_el, "x"_el, HttpCookieInsertionOptions{}.setSecure(false)));
            REQUIRE_THROWS(
                jar.setCookie(idnaUrl, "domain"_el, "x"_el, HttpCookieInsertionOptions{}.setDomain("com"_el)));
        });
        static_cast<void>(loop->runOnce());
    }
};
