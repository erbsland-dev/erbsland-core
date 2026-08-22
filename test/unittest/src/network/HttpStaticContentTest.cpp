// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/network/http_server/HttpMediaTypeMapping.hpp>
#include <erbsland/network/http_server/HttpStaticFileHandler.hpp>
#include <erbsland/network/http_server/HttpStaticResourceHandler.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(
    HttpMediaTypeMapping HttpStaticContent HttpStaticContentHandler HttpStaticFileHandler HttpStaticResourceHandler)
class HttpStaticContentTest final : public el::UnitTest {
public:
    void testMediaTypeMappingCopiesAndLongestSuffix() {
        const auto empty = HttpMediaTypeMapping::create();
        REQUIRE_EQUAL(empty->mediaType("archive.tar.gz"_el).toString(), "application/octet-stream"_el);
        empty->setSuffix(".GZ"_el, "application/gzip"_el)
            .setSuffix(".tar.gz"_el, "application/x-tar-gzip"_el)
            .setFallbackMediaType("application/x-fallback"_el);
        REQUIRE_EQUAL(empty->mediaType("ARCHIVE.TAR.GZ"_el).toString(), "application/x-tar-gzip"_el);
        REQUIRE_EQUAL(empty->mediaType("archive.gz"_el).toString(), "application/gzip"_el);
        REQUIRE_EQUAL(empty->mediaType("unknown"_el).toString(), "application/x-fallback"_el);

        const auto copy = empty->copy();
        copy->removeSuffix(".tar.gz"_el).setSuffix(".txt"_el, "text/x-test"_el);
        REQUIRE_EQUAL(copy->mediaType("archive.tar.gz"_el).toString(), "application/gzip"_el);
        REQUIRE_EQUAL(empty->mediaType("archive.tar.gz"_el).toString(), "application/x-tar-gzip"_el);
        copy->clear();
        REQUIRE_EQUAL(copy->mediaType("note.txt"_el).toString(), "application/x-fallback"_el);

        REQUIRE_EQUAL(HttpMediaTypeMapping::defaultMapping()->mediaType("INDEX.HTML"_el).toString(), "text/html"_el);
        REQUIRE_THROWS_AS(el::err::ParameterError, empty->setSuffix("txt"_el, "text/plain"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, empty->setSuffix(".bad/path"_el, "text/plain"_el));
        REQUIRE_THROWS_AS(el::err::ParseError, empty->setSuffix(".txt"_el, "not a media type"_el));
    }

    void testSharedHandlerConfiguration() {
        auto handler = HttpStaticFileHandler::create(el::path::Path{"."_el}, "/assets/"_el);
        const auto defaultIndexes = el::text::StringList{{"index.html"_el, "index.htm"_el}};
        REQUIRE_EQUAL(handler->urlPrefix(), "/assets"_el);
        REQUIRE_EQUAL(handler->priority(), 0);
        REQUIRE_EQUAL(handler->indexFileNames(), defaultIndexes);

        const auto mapping = HttpMediaTypeMapping::create();
        handler->setUrlPrefix("/public/"_el)
            .setPriority(42)
            .setIndexFileNames(el::text::StringList{{"home.html"_el}})
            .setMediaTypeMapping(mapping);
        REQUIRE_EQUAL(handler->urlPrefix(), "/public"_el);
        REQUIRE_EQUAL(handler->priority(), 42);
        REQUIRE_EQUAL(handler->indexFileNames(), el::text::StringList{{"home.html"_el}});
        REQUIRE_EQUAL(handler->mediaTypeMapping(), mapping);

        REQUIRE_THROWS_AS(
            el::err::ParameterError, handler->setIndexFileNames(el::text::StringList{{"../index.html"_el}}));
        REQUIRE_THROWS_AS(el::err::ParameterError, handler->setMediaTypeMapping({}));
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpStaticResourceHandler::create("_invalid"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, HttpStaticResourceHandler::create("invalid/name"_el));
    }
};
