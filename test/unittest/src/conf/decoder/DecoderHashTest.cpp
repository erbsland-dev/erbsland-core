// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/char/CharStream.hpp>
#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using el::conf::impl::CharStream;
using el::conf::impl::CharStreamPtr;
using el::conf::impl::DecodedChar;
using namespace el::text::literals;
namespace nc = el::conf::impl::nc;

TESTED_TARGETS(Decoder Hasher)
class DecoderHashTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    CharStreamPtr decoder;
    DecodedChar decodedChar;

    void tearDown() override { cleanUpTestFileDirectory(); }

    void verifyHash(const el::text::String &content, const el::mem::ByteBlock &expectedDigest) {
        auto source = createTestMemorySource(content);
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        decodedChar = decoder->next();
        while (!decodedChar.character().isEndOfData()) {
            decodedChar = decoder->next();
        }
        REQUIRE_EQUAL(decoder->digest(), expectedDigest);
    }

    void testNoHash() {
        WITH_CONTEXT(verifyHash("[main]\nvalue: 123\nanother value: \"example\"\n"_el, el::mem::ByteBlock{}));
    }

    void testWithHash() {
        // verify the used algorithm.
        REQUIRE(el::conf::impl::defaults::documentHashAlgorithm == el::cryptology::HashAlgorithm::Sha3_256);
        WITH_CONTEXT(verifyHash(
            "@signature \"...\"\n[main]\nvalue: 123\nanother value: \"example\"\n"_el,
            bytesFromHex("b352bf8f49d930ec1267659eddaee1a1a6f38840e7d67ef5733ca2cee83f6633"_el)));
        WITH_CONTEXT(verifyHash(
            "@signature \"only signature line changes\"\n[main]\nvalue: 123\nanother value: \"example\"\n"_el,
            bytesFromHex("b352bf8f49d930ec1267659eddaee1a1a6f38840e7d67ef5733ca2cee83f6633"_el)));
        WITH_CONTEXT(verifyHash(
            "@signature \"...\"\n[main] #comment\nvalue: 123\nanother value: \"example\"\n"_el,
            bytesFromHex("217a5a3718139d97c00d4d9e28cbae75cb5694fd578d9d3e3130e59e57974104"_el)));
    }

    void testSignatureInSecondLine() {
        WITH_CONTEXT(verifyHash(
            "# comment\n@signature \"...\"\n[main]\nvalue: 123\nanother value: \"example\"\n"_el,
            el::mem::ByteBlock{}));
    }

    void testDocumentWithBOM() {
        const auto content =
            el::text::String{"@signature \"...\"\n[main]\nvalue: 123\nanother value: \"example\"\n"_el};
        const auto encoded =
            el::text::StringEncoder{content}.encode(el::text::StringEncoding::Utf8, el::text::StringBomMode::Require);
        const auto testFile = createTestFile(encoded);
        auto source = Source::fromFile(el::path::Path{testFile});
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        decodedChar = decoder->next();
        while (!decodedChar.character().isEndOfData()) {
            decodedChar = decoder->next();
        }
        REQUIRE_EQUAL(
            decoder->digest(), bytesFromHex("b352bf8f49d930ec1267659eddaee1a1a6f38840e7d67ef5733ca2cee83f6633"_el));
    }
};
