// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/constants/Limits.hpp>
#include <erbsland/conf/impl/source/FileSource.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/text/String.hpp>

#include <filesystem>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Source)
class FileSourceTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    SourcePtr source;

    void tearDown() override { cleanUpTestFileDirectory(); }

    void testConstructionAndState() {
        const auto filePath = createTestFile("[main]"_el);
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE(source);
        REQUIRE_EQUAL(source->name(), "file"_el);
        REQUIRE_FALSE(source->isOpen());
        REQUIRE_FALSE(source->atEnd());

        REQUIRE_NOTHROW(source->open());
        REQUIRE(source->isOpen());
        source->close();
        REQUIRE_FALSE(source->isOpen());
        REQUIRE_FALSE(source->atEnd());
    }

    void testInvalidAndNonRegularPaths() {
        source = Source::fromFile(el::path::Path{std::filesystem::path{"/this/path/does/not/exist"}});
        try {
            source->open();
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_FALSE(error.cause());
        }

        const auto directory = useTestFileDirectory();
        source = Source::fromFile(el::path::Path{directory});
        REQUIRE_THROWS_AS(ConfError, source->open());
    }

    void testLineEndingsAndFinalLine() {
        const auto filePath = createTestFile("one\ntwo\r\n\nfour"_el);
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine(), el::text::String{"one\n"});
        REQUIRE_EQUAL(source->readLine(), el::text::String{"two\r\n"});
        REQUIRE_EQUAL(source->readLine(), el::text::String{"\n"});
        REQUIRE_EQUAL(source->readLine(), el::text::String{"four"});
        REQUIRE_FALSE(source->atEnd());
        REQUIRE(source->readLine().isEmpty());
        REQUIRE(source->atEnd());
        REQUIRE_FALSE(source->isOpen());
    }

    void testEmptyFile() {
        const auto filePath = createTestFile(el::text::String{});
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_NOTHROW(source->open());
        REQUIRE(source->readLine().isEmpty());
        REQUIRE(source->atEnd());
    }

    void testClosedSourceErrors() {
        const auto filePath = createTestFile("line\nnext\n"_el);
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_THROWS_AS(ConfError, source->readLine());
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine(), el::text::String{"line\n"});
        source->close();
        REQUIRE_THROWS_AS(ConfError, source->readLine());
    }

    void requireLengthAccepted(const std::size_t byteLength, const bool withLineEnding) {
        auto bytes = std::vector<uint8_t>(byteLength, static_cast<uint8_t>('a'));
        if (withLineEnding) {
            bytes.back() = static_cast<uint8_t>('\n');
        }
        const auto filePath = createTestFile(el::mem::ByteBlock::fromVector(bytes));
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine().length().toSizeT(), byteLength);
    }

    void requireLengthRejected(const std::size_t byteLength, const bool withLineEnding) {
        auto bytes = std::vector<uint8_t>(byteLength, static_cast<uint8_t>('b'));
        if (withLineEnding) {
            bytes.back() = static_cast<uint8_t>('\n');
        }
        const auto filePath = createTestFile(el::mem::ByteBlock::fromVector(bytes));
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_NOTHROW(source->open());
        try {
            static_cast<void>(source->readLine());
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::LimitExceeded);
        }
        REQUIRE_FALSE(source->isOpen());
    }

    void testByteLengthBoundaries() {
        WITH_CONTEXT(requireLengthAccepted(el::conf::impl::limits::maxLineLength, false));
        WITH_CONTEXT(requireLengthAccepted(el::conf::impl::limits::maxLineLength, true));
        WITH_CONTEXT(requireLengthRejected(el::conf::impl::limits::maxLineLength + 1, false));
        WITH_CONTEXT(requireLengthRejected(el::conf::impl::limits::maxLineLength + 1, true));
    }

    void testBomIsConsumed() {
        const auto bytes = el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0xef, 0xbb, 0xbf, 'a', 'b', 'c', '\n'});
        const auto filePath = createTestFile(bytes);
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine(), el::text::String{"abc\n"});
    }

    void testMalformedUtf8IsTranslated() {
        const auto bytes = el::mem::ByteBlock::fromVector(std::vector<uint8_t>{'a', 0x80, '\n'});
        const auto filePath = createTestFile(bytes);
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_NOTHROW(source->open());
        try {
            static_cast<void>(source->readLine());
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::Encoding);
        }
    }

    void testReturnedLinesKeepTheirStorageAlive() {
        const auto filePath = createTestFile("first\nsecond\n"_el);
        source = Source::fromFile(el::path::Path{filePath});
        REQUIRE_NOTHROW(source->open());
        const auto first = source->readLine();
        const auto second = source->readLine();
        source.reset();
        REQUIRE_EQUAL(first, el::text::String{"first\n"});
        REQUIRE_EQUAL(second, el::text::String{"second\n"});
    }

    void testCodeSnippetReopensSmallClosedFile() {
        const auto filePath = createTestFile("zero\none\ntwo\nthree\nfour\nfive\n"_el);
        source = Source::fromFile(el::path::Path{filePath});

        const auto snippet =
            source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{3U}, el::unit::ColumnIndex{1U}});
        REQUIRE(snippet.has_value());
        REQUIRE_EQUAL(snippet->startLine, el::unit::LineIndex{1U});
        REQUIRE_EQUAL(snippet->lines.count(), el::unit::ItemCount{5U});
        REQUIRE_EQUAL(snippet->lines.get(el::unit::ItemIndex::zero()), "one"_el);
        REQUIRE_EQUAL(snippet->lines.get(el::unit::ItemIndex{4U}), "five"_el);

        REQUIRE_FALSE(source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{10'000U}}).has_value());
    }

    void testCodeSnippetUsesRecentFilePositions() {
        auto bytes = std::vector<uint8_t>{};
        for (auto line = 0U; line < 30U; ++line) {
            bytes.insert(bytes.end(), {'l', 'i', 'n', 'e', '\n'});
        }
        const auto filePath = createTestFile(el::mem::ByteBlock::fromVector(bytes));
        source = Source::fromFile(el::path::Path{filePath});
        source->open();
        while (!source->readLine().isEmpty()) {}

        bytes.front() = 0x80U;
        auto writeOptions = el::path::PathWriteDataOptions{};
        writeOptions.setCreationMode(el::path::PathCreateMode::CreateOrOverwrite);
        el::path::Path{filePath}.content().writeDataOrThrow(el::mem::ByteBlock::fromVector(bytes), writeOptions);
        const auto snippet =
            source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{22U}, el::unit::ColumnIndex::zero()});
        REQUIRE(snippet.has_value());
        REQUIRE_EQUAL(snippet->startLine, el::unit::LineIndex{20U});
        REQUIRE_EQUAL(snippet->lines.count(), el::unit::ItemCount{5U});
    }

    void testFileErrorsReceiveCodeSnippetsAfterParsing() {
        const auto filePath = createTestFile("[main]\nvalue: \"text\"\n"_el);
        const auto document = Parser{}.parseFileOrThrow(el::path::Path{filePath});
        try {
            static_cast<void>(document->getIntegerOrThrow("main.value"_el));
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE(error.context().filePath().has_value());
            REQUIRE(error.context().location().has_value());
            REQUIRE(error.context().codeSnippet().has_value());
            REQUIRE_EQUAL(error.context().codeSnippet()->lines.count(), el::unit::ItemCount{2U});
        }
    }

    void testValidatedSecretFileValuesSuppressCodeSnippets() {
        const auto ruleDocument = Parser{}.parseTextOrThrow(
            "[credentials.password]\n"
            "type: \"text\"\n"
            "is_secret: yes\n"
            "chars: \"(a-z)\"\n"_el);
        const auto rules = vr::Rules::createFromDocument(ruleDocument);
        const auto filePath = createTestFile("[credentials]\npassword: \"SECRET\"\n"_el);
        const auto document = Parser{}.parseFileOrThrow(el::path::Path{filePath});
        try {
            rules->validate(document, 1);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE(error.context().filePath().has_value());
            REQUIRE(error.context().location().has_value());
            REQUIRE_FALSE(error.context().codeSnippet().has_value());
        }

        const auto validFilePath = createTestFile("[credentials]\npassword: \"secret\"\n"_el);
        const auto validDocument = Parser{}.parseFileOrThrow(el::path::Path{validFilePath});
        REQUIRE_NOTHROW(rules->validate(validDocument, 1));
        try {
            static_cast<void>(validDocument->getIntegerOrThrow("credentials.password"_el));
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE(error.context().filePath().has_value());
            REQUIRE(error.context().location().has_value());
            REQUIRE_FALSE(error.context().codeSnippet().has_value());
        }
    }
};
