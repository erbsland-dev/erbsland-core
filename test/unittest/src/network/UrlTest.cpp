// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/network/url/Url.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(Url UrlScheme UrlParseOptions UrlFormatOptions)
class UrlTest final : public el::UnitTest {
public:
    void testInvalidAndNetworkUrls() {
        const auto invalid = Url{};
        REQUIRE_FALSE(invalid.isValid());
        REQUIRE_EQUAL(invalid.scheme(), UrlScheme::Invalid);
        REQUIRE(invalid.toString().isEmpty());

        const auto parsed = Url::fromStringOrThrow("HTTPS://user:p%40ss@WWW.bücher.example/a%20b?q=x+y#part"_el);
        REQUIRE(parsed.isValid());
        REQUIRE_EQUAL(parsed.scheme(), UrlScheme::Https);
        REQUIRE(parsed.isSecureScheme());
        REQUIRE_EQUAL(parsed.schemeText(), "https"_el);
        REQUIRE_EQUAL(parsed.username(), "user"_el);
        REQUIRE_EQUAL(parsed.password(), "p@ss"_el);
        REQUIRE_EQUAL(parsed.endpoint().port(), Port{443U});
        REQUIRE_EQUAL(parsed.path(), "/a b"_el);
        REQUIRE_EQUAL(parsed.query(), "q=x+y"_el);
        REQUIRE_EQUAL(parsed.fragment(), "part"_el);
        REQUIRE_EQUAL(parsed.authorityText(), "user:p%40ss@WWW.bücher.example"_el);
        REQUIRE_EQUAL(parsed.toString(), "https://user:***@www.xn--bcher-kva.example/a%20b?q=x+y#part"_el);

        const auto revealed = UrlFormatOptions{}
                                  .setRedactPassword(false)
                                  .setIncludeDefaultPort(true)
                                  .setIncludeFragment(false)
                                  .setHostNameFormat(HostNameFormat::Unicode);
        REQUIRE_EQUAL(parsed.toString(revealed), "https://user:p%40ss@www.bücher.example:443/a%20b?q=x+y"_el);

        REQUIRE_EQUAL(Url::fromStringOrThrow("http://127.0.0.1/"_el).endpoint().port(), Port{80U});
        REQUIRE_EQUAL(Url::fromStringOrThrow("ftp://example.test/file"_el).endpoint().port(), Port{21U});
        REQUIRE_EQUAL(Url::fromStringOrThrow("ftps://example.test/file"_el).endpoint().port(), Port{990U});
        REQUIRE_EQUAL(
            Url::fromStringOrThrow("https://example.test:8443/path"_el).toString(),
            "https://example.test:8443/path"_el);
        REQUIRE_EQUAL(
            Url::fromStringOrThrow("https://www.b%C3%BCcher.example/path"_el).toString(),
            "https://www.xn--bcher-kva.example/path"_el);
    }

    void testComponentConstruction() {
        const auto endpoint = HostEndpoint::fromStringOrThrow("example.test:8443"_el);
        const auto secure = Url{endpoint, "/v1"_el};
        REQUIRE_EQUAL(secure.scheme(), UrlScheme::Https);
        REQUIRE_EQUAL(secure.toString(), "https://example.test:8443/v1"_el);
        REQUIRE_THROWS_AS(el::err::ParameterError, Url(UrlScheme::File, endpoint));
        REQUIRE_THROWS_AS(el::err::ParameterError, Url(UrlScheme::Http, HostEndpoint{}));
        REQUIRE_THROWS_AS(el::err::ParameterError, Url(endpoint, "path-without-slash"_el));

        const auto file = Url::file(Host::fromStringOrThrow("server"_el), "/share"_el, "download=1"_el, "top"_el);
        REQUIRE_EQUAL(file.toString(), "file://server/share?download=1#top"_el);
        REQUIRE_THROWS_AS(
            el::err::ParameterError, Url::customWithAuthority("demo"_el, "host"_el, "path-without-slash"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, Url::custom("https"_el, "value"_el));
    }

    void testFileMailtoAndCustomUrls() {
        const auto localFile = Url::fromStringOrThrow("file:///tmp/a%20b"_el);
        REQUIRE_EQUAL(localFile.path(), "/tmp/a b"_el);
        REQUIRE_EQUAL(localFile.toString(), "file:///tmp/a%20b"_el);

        const auto hostedFile = Url::fromStringOrThrow("file://server/share/file"_el);
        REQUIRE_EQUAL(hostedFile.endpoint().host().toString(), "server"_el);
        REQUIRE_EQUAL(hostedFile.toString(), "file://server/share/file"_el);
        REQUIRE_FALSE(Url::fromString("file://user@server/share"_el).isValid());
        REQUIRE_FALSE(Url::fromString("file://server:42/share"_el).isValid());

        const auto mail = Url::fromStringOrThrow("mailto:user%40example.test?subject=Hello%20World"_el);
        REQUIRE_EQUAL(mail.path(), "user@example.test"_el);
        REQUIRE_EQUAL(mail.toString(), "mailto:user@example.test?subject=Hello%20World"_el);
        REQUIRE_FALSE(Url::fromString("mailto:/user@example.test"_el).isValid());
        REQUIRE_FALSE(Url::fromString("mailto:user%2Fname@example.test"_el).isValid());
        REQUIRE_THROWS_AS(el::err::ParameterError, Url::mailto("user/name@example.test"_el));

        REQUIRE_EQUAL(Url::fromStringOrThrow("demo:value#part"_el).toString(), "demo:value#part"_el);
        REQUIRE_EQUAL(Url::fromStringOrThrow("demo:/value"_el).toString(), "demo:/value"_el);

        const auto custom = Url::fromStringOrThrow("demo://host.test:42/a/../b"_el);
        REQUIRE_EQUAL(custom.scheme(), UrlScheme::Custom);
        REQUIRE_EQUAL(custom.endpoint().port(), Port{42U});
        REQUIRE_EQUAL(custom.path(), "/a/../b"_el);
        REQUIRE_EQUAL(custom.toString(), "demo://host.test:42/a/../b"_el);

        const auto credentials = Url::fromStringOrThrow("demo://name:secret@host.test:42/value"_el);
        REQUIRE_EQUAL(credentials.username(), "name"_el);
        REQUIRE_EQUAL(credentials.password(), "secret"_el);
        REQUIRE_EQUAL(credentials.toString(), "demo://name:***@host.test:42/value"_el);

        const auto opaque = Url::fromStringOrThrow("demo://not a host/value"_el);
        REQUIRE_EQUAL(opaque.authorityText(), "not a host"_el);
        REQUIRE_EQUAL(opaque.endpoint(), HostEndpoint{});
        REQUIRE_EQUAL(opaque.toString(), "demo://***/value"_el);
        REQUIRE_EQUAL(opaque.toString(UrlFormatOptions{}.setRedactPassword(false)), "demo://not a host/value"_el);
    }

    void testNormalizationAndLimits() {
        const auto normalized = Url::fromStringOrThrow("https://example.test/cafe%CC%81"_el);
        REQUIRE_EQUAL(normalized.path(), u8"/café"_el);
        REQUIRE_EQUAL(normalized.toString(), "https://example.test/caf%C3%A9"_el);
        REQUIRE_FALSE(Url::fromString("https://example.test/%GG"_el).isValid());
        REQUIRE_FALSE(Url::fromString("https://example.test/%C3"_el).isValid());
        REQUIRE_FALSE(Url::fromString("relative/path"_el).isValid());
        REQUIRE_THROWS_AS(el::err::ParseError, Url::fromStringOrThrow("https:/example.test"_el));

        const auto scoped = Url::fromStringOrThrow("https://[fe80::1%257]/path"_el);
        REQUIRE_EQUAL(scoped.endpoint().scopeId().toRawValue(), uint32_t{7U});
        REQUIRE_EQUAL(scoped.toString(), "https://[fe80::1%257]/path"_el);
        REQUIRE_FALSE(Url::fromString("https://[fe80::1%7]/path"_el).isValid());

        auto boundary = el::text::StringEditor{"https://example.test/"_el};
        boundary.append(
            "x"_el,
            el::unit::ItemCount{UrlParseOptions::cDefaultMaximumLength.toRawValue() - boundary.length().toRawValue()});
        REQUIRE(Url::fromString(el::text::String{boundary}).isValid());
        boundary.append(U'x');
        REQUIRE_FALSE(Url::fromString(el::text::String{boundary}).isValid());
    }

    void testRelativeResolution() {
        const auto base = Url::fromStringOrThrow("http://a.test/b/c/d;p?q#old"_el);
        requireResolved(base, "g"_el, "http://a.test/b/c/g"_el);
        requireResolved(base, "./g"_el, "http://a.test/b/c/g"_el);
        requireResolved(base, "g/"_el, "http://a.test/b/c/g/"_el);
        requireResolved(base, "/g"_el, "http://a.test/g"_el);
        requireResolved(base, "?y"_el, "http://a.test/b/c/d;p?y"_el);
        requireResolved(base, "#s"_el, "http://a.test/b/c/d;p?q#s"_el);
        requireResolved(base, ""_el, "http://a.test/b/c/d;p?q"_el);
        requireResolved(base, "."_el, "http://a.test/b/c/"_el);
        requireResolved(base, ".."_el, "http://a.test/b/"_el);
        requireResolved(base, "../g"_el, "http://a.test/b/g"_el);
        requireResolved(base, "../../g"_el, "http://a.test/g"_el);
        requireResolved(base, "../../../g"_el, "http://a.test/g"_el);
        requireResolved(base, "g?y#s"_el, "http://a.test/b/c/g?y#s"_el);
        requireResolved(base, "//g.test/x"_el, "http://g.test/x"_el);
        requireResolved(base, "https://other.test/a/../b"_el, "https://other.test/b"_el);

        const auto idna = base.resolvedOrThrow("//bücher.example/x"_el);
        REQUIRE_EQUAL(idna.toString(), "http://xn--bcher-kva.example/x"_el);
        const auto ipv6 = base.resolvedOrThrow("//[2001:db8::1]/x"_el);
        REQUIRE_EQUAL(ipv6.toString(), "http://[2001:db8::1]/x"_el);

        REQUIRE_EQUAL(
            Url::fromStringOrThrow("ftp://example.test/a/b"_el).resolvedOrThrow("../c"_el).toString(),
            "ftp://example.test/c"_el);
        REQUIRE_EQUAL(Url::fromStringOrThrow("file:///a/b"_el).resolvedOrThrow("../c"_el).toString(), "file:///c"_el);
        REQUIRE_FALSE(base.resolved("abc"_el, UrlParseOptions{}.setMaximumLength(el::unit::ByteLength{2U})).isValid());
    }

    void testRfc3986ResolutionExamples() {
        const auto base = Url::fromStringOrThrow("http://a/b/c/d;p?q"_el);

        requireResolved(base, "g:h"_el, "g:h"_el);
        requireResolved(base, ";x"_el, "http://a/b/c/;x"_el);
        requireResolved(base, "g;x"_el, "http://a/b/c/g;x"_el);
        requireResolved(base, "g;x?y#s"_el, "http://a/b/c/g;x?y#s"_el);
        requireResolved(base, "./"_el, "http://a/b/c/"_el);
        requireResolved(base, "../"_el, "http://a/b/"_el);
        requireResolved(base, "../.."_el, "http://a/"_el);
        requireResolved(base, "../../"_el, "http://a/"_el);
        requireResolved(base, "../../../../g"_el, "http://a/g"_el);
        requireResolved(base, "/./g"_el, "http://a/g"_el);
        requireResolved(base, "/../g"_el, "http://a/g"_el);
        requireResolved(base, "g."_el, "http://a/b/c/g."_el);
        requireResolved(base, ".g"_el, "http://a/b/c/.g"_el);
        requireResolved(base, "g.."_el, "http://a/b/c/g.."_el);
        requireResolved(base, "..g"_el, "http://a/b/c/..g"_el);
        requireResolved(base, "./../g"_el, "http://a/b/g"_el);
        requireResolved(base, "./g/."_el, "http://a/b/c/g/"_el);
        requireResolved(base, "g/./h"_el, "http://a/b/c/g/h"_el);
        requireResolved(base, "g/../h"_el, "http://a/b/c/h"_el);
        requireResolved(base, "g;x=1/./y"_el, "http://a/b/c/g;x=1/y"_el);
        requireResolved(base, "g;x=1/../y"_el, "http://a/b/c/y"_el);
        requireResolved(base, "g?y/./x"_el, "http://a/b/c/g?y/./x"_el);
        requireResolved(base, "g?y/../x"_el, "http://a/b/c/g?y/../x"_el);
        requireResolved(base, "g#s/./x"_el, "http://a/b/c/g#s/./x"_el);
        requireResolved(base, "g#s/../x"_el, "http://a/b/c/g#s/../x"_el);
    }

    void testEmptyQueryAndFragment() {
        const auto parsed = Url::fromStringOrThrow("https://example.test/path?#"_el);
        REQUIRE(parsed.hasQuery());
        REQUIRE(parsed.query().isEmpty());
        REQUIRE(parsed.hasFragment());
        REQUIRE(parsed.fragment().isEmpty());
        REQUIRE_EQUAL(parsed.toString(), "https://example.test/path?#"_el);

        const auto base = Url::fromStringOrThrow("https://example.test/path?old#part"_el);
        const auto emptyQuery = base.resolvedOrThrow("?"_el);
        REQUIRE(emptyQuery.hasQuery());
        REQUIRE(emptyQuery.query().isEmpty());
        REQUIRE_FALSE(emptyQuery.hasFragment());
        REQUIRE_EQUAL(emptyQuery.toString(), "https://example.test/path?"_el);
        const auto emptyFragment = base.resolvedOrThrow("#"_el);
        REQUIRE(emptyFragment.hasFragment());
        REQUIRE_EQUAL(emptyFragment.toString(), "https://example.test/path?old#"_el);

        REQUIRE_FALSE(Url::mailto("a@example.test"_el).resolved("next"_el).isValid());
        REQUIRE_FALSE(Url::custom("demo"_el, "path"_el).resolved("next"_el).isValid());
        REQUIRE_FALSE(Url{}.resolved("https://example.test/"_el).isValid());
        const auto custom = Url::customWithAuthority("demo"_el, "host.test:42"_el, "/a/b"_el);
        REQUIRE_EQUAL(custom.resolvedOrThrow("../c"_el).toString(), "demo://host.test:42/c"_el);
    }

private:
    void requireResolved(const Url &base, const el::text::String &reference, const el::text::String &expected) {
        REQUIRE_EQUAL(base.resolvedOrThrow(reference).toString(), expected);
    }
};
