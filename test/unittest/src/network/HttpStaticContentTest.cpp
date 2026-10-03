// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteInputStream.hpp>
#include <erbsland/core/impl/application_data/ApplicationResourceData.hpp>
#include <erbsland/core/impl/ApplicationDataAccess.hpp>
#include <erbsland/HttpMediaTypeMapping.hpp>
#include <erbsland/HttpStaticContent.hpp>
#include <erbsland/HttpStaticFileHandler.hpp>
#include <erbsland/HttpStaticResourceHandler.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(
    HttpMediaTypeMapping HttpStaticContent HttpStaticContentHandler HttpStaticFileHandler HttpStaticResourceHandler)
class HttpStaticContentTest final : public el::UnitTest {
private:
    void requireRejectedIndexFileName(const el::text::String &name) {
        const auto handler = HttpStaticFileHandler::create(el::path::Path{"."_el});
        const auto originalNames = handler->indexFileNames();
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, handler->setIndexFileNames(el::text::StringList{"home.html"_el, name}));
        REQUIRE_EQUAL(handler->indexFileNames(), originalNames);
    }

public:
    void testIndexFileNamesValidateTheEntireSegment() {
        WITH_CONTEXT(requireRejectedIndexFileName("index\0.html"_el));
        WITH_CONTEXT(requireRejectedIndexFileName("index.\0html"_el));
        WITH_CONTEXT(requireRejectedIndexFileName("index.html\0"_el));
        WITH_CONTEXT(requireRejectedIndexFileName("index.html/child"_el));
        WITH_CONTEXT(requireRejectedIndexFileName("index.html\\child"_el));
        WITH_CONTEXT(requireRejectedIndexFileName("index.html:stream"_el));
        WITH_CONTEXT(requireRejectedIndexFileName("NUL.html"_el));
        WITH_CONTEXT(requireRejectedIndexFileName("COM1.backup.html"_el));

        const auto handler = HttpStaticFileHandler::create(el::path::Path{"."_el});
        const auto validNames = el::text::StringList{"index.backup.html"_el, ".index.html"_el, "nulled.html"_el};
        handler->setIndexFileNames(validNames);
        REQUIRE_EQUAL(handler->indexFileNames(), validNames);
    }

    void testApplicationResourceContentRetainsOnlyItsProvider() {
        auto handler = HttpStaticResourceHandlerPtr{};
        auto weakData = std::weak_ptr<el::core::impl::ApplicationData>{};
        auto weakResources = std::weak_ptr<el::core::impl::ApplicationResourceData>{};
        {
            const auto scope = ApplicationTestScope<>{};
            const auto data = el::core::impl::applicationData();
            weakData = data;
            weakResources = data->resources().get();
            handler = HttpStaticResourceHandler::create("test"_el);
        }
        REQUIRE(weakData.expired());
        REQUIRE_FALSE(weakResources.expired());
        const auto path = el::path::Path{"plain.txt"_el};
        REQUIRE(handler->hasPath(path));
        auto content = handler->getContent(path);
        handler.reset();
        REQUIRE_FALSE(weakResources.expired());
        REQUIRE_EQUAL(content->length(), el::unit::ByteCount{5U});
        REQUIRE(content->open() != nullptr);
        content.reset();
        REQUIRE(weakResources.expired());
    }

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
        REQUIRE_THROWS_AS(el::err::ParameterFailure, empty->setSuffix("txt"_el, "text/plain"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, empty->setSuffix(".bad/path"_el, "text/plain"_el));
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
            el::err::ParameterFailure, handler->setIndexFileNames(el::text::StringList{{"../index.html"_el}}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, handler->setMediaTypeMapping({}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, HttpStaticResourceHandler::create("_invalid"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, HttpStaticResourceHandler::create("invalid/name"_el));
    }
};
