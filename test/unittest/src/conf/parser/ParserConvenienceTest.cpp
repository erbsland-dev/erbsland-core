// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserTestHelper.hpp"

#include <erbsland/conf/StdFormatForConf.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Parser)
class ParserConvenienceTest final : public UNITTEST_SUBCLASS(ParserTestHelper) {
public:
    Parser parser;
    std::unique_ptr<ConfError> error;

    void tearDown() override {
        cleanUpTestFileDirectory();
        error = {};
        doc = {};
    }

    void verifyTextSource(const ConfErrorContext &context) {
        REQUIRE_FALSE(context.title().isEmpty());
        REQUIRE_FALSE(context.description().isEmpty());
        REQUIRE_FALSE(context.filePath().has_value());
        REQUIRE(context.location().has_value());
        REQUIRE(context.codeSnippet().has_value());
    }

    void verifyFileSource(const ConfErrorContext &context, const std::filesystem::path &path) {
        const auto expectedPath = el::path::Path{std::filesystem::absolute(path)}.toString();
        REQUIRE(context.filePath().has_value());
        REQUIRE_EQUAL(context.filePath()->toString(), expectedPath);
        REQUIRE(context.location().has_value());
        REQUIRE(context.codeSnippet().has_value());
    }

    void requireTextSourceAfterError() {
        REQUIRE(doc == nullptr);
        error = std::make_unique<ConfError>(parser.lastError());
        verifyTextSource(error->context());
    }

    template <typename Fn>
    void requireFileSourceAfterError(const Fn &fn) {
        try {
            fn();
            REQUIRE(false);
        } catch (const ConfError &error) {
            verifyTextSource(error.context());
        }
    }

    void testParseTextUsesTextSourceAndParse() {
        const auto text = el::text::String{"["_el};

        REQUIRE_NOTHROW(doc = parser.parseText(text));
        WITH_CONTEXT(requireTextSourceAfterError());

        REQUIRE_NOTHROW(doc = parser.parseText("["_el));
        WITH_CONTEXT(requireTextSourceAfterError());
    }

    void testParseTextOrThrowUsesTextSource() {
        const auto text = el::text::String{"["_el};

        WITH_CONTEXT(requireFileSourceAfterError([&]() -> void { doc = parser.parseTextOrThrow(text); }));
        WITH_CONTEXT(requireFileSourceAfterError([&]() -> void { doc = parser.parseTextOrThrow("["_el); }));
    }

    void testParseFileUsesFileSourceAndParse() {
        const auto filePath = createTestFile("config/invalid.elcl", "["_el);
        REQUIRE_NOTHROW(doc = parser.parseFile(el::path::Path{filePath}));
        REQUIRE(doc == nullptr);
        error = std::make_unique<ConfError>(parser.lastError());
        verifyFileSource(error->context(), filePath);
    }

    void testParseFileOrThrowUsesFileSource() {
        const auto filePath = createTestFile("config/invalid.elcl", "["_el);
        try {
            doc = parser.parseFileOrThrow(el::path::Path{filePath});
            REQUIRE(false);
        } catch (const ConfError &error) {
            verifyFileSource(error.context(), filePath);
        }
    }

    void testSuccessfulParseClearsLastErrorContext() {
        REQUIRE(parser.parseText("["_el) == nullptr);
        REQUIRE_FALSE(parser.lastError().title().isEmpty());
        REQUIRE(parser.parseText({}) != nullptr);
        REQUIRE(parser.lastError().title().isEmpty());
        REQUIRE(parser.lastError().description().isEmpty());
    }
};
