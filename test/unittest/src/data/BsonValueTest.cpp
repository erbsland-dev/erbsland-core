// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bson/BsonValue.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/OutOfRangeError.hpp>
#include <erbsland/OutOfRangeFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <vector>

using namespace el::text::literals;
using namespace el::data::bson;
TESTED_TARGETS(BsonValue BsonParseOptions BsonFormatOptions)
class BsonValueTest final : public el::UnitTest {
private:
    static constexpr auto cAField = "a"_el;
    static constexpr auto cEditorField = "editor"_el;
    static constexpr auto cLiteralField = "literal"_el;
    static constexpr auto cMissingField = "missing"_el;
    static constexpr auto cPartsField = "parts"_el;
    static constexpr auto cRawField = "raw"_el;
    /// BSON field with multibyte UTF-8 characters in its name.
    static constexpr auto cUnicodeField = "Grüße"_el;

private:
    static auto bytes(std::initializer_list<uint8_t> data) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromVector(std::vector<uint8_t>{data});
    }

    void requireRejectedKey(const el::text::String &key) {
        auto document = BsonValue{BsonDocument{}};
        document.set(key, BsonValue{int32_t{1}});
        REQUIRE_THROWS_AS(el::err::ParameterFailure, document.toByteBlock());
    }

public:
    void testImplicitTextArguments() {
        BsonValue literal = cLiteralField;
        BsonValue utf8Literal = u8"Grüße"_el;
        auto editor = el::StringEditor{"before"_el};
        BsonValue edited = editor;
        auto object = BsonValue{BsonDocument{}};
        object.set(cLiteralField, "value"_el).set(cEditorField, editor);
        auto array = BsonValue{BsonArray{}};
        array.append("item"_el).append(editor);
        editor.append(" after"_el);
        REQUIRE_EQUAL(literal.getText().value(), cLiteralField);
        REQUIRE_EQUAL(utf8Literal.getText().value(), "Grüße"_el);
        REQUIRE_EQUAL(edited.getText().value(), "before"_el);
        REQUIRE_EQUAL(object.getOrThrow(cLiteralField).getText().value(), "value"_el);
        REQUIRE_EQUAL(object.getOrThrow(cEditorField).getText().value(), "before"_el);
        REQUIRE_EQUAL(array.getOrThrow(el::ItemIndex{1}).getText().value(), "before"_el);
    }

    void testLookupExceptionContracts() {
        const auto object = BsonValue{BsonDocument{}};
        REQUIRE_THROWS_AS(el::err::NotFoundError, object.getOrThrow(cMissingField));
        auto array = BsonValue{BsonArray{}};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, array.getOrThrow(el::unit::ItemIndex{0U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeFailure, array.set(el::unit::ItemIndex{1U}, BsonValue{}));
        REQUIRE_THROWS_AS(el::err::LogicFailure, array.getOrThrow(cMissingField));
    }

    void testTruncatedInputIsAParseFailure() {
        auto document = BsonValue{BsonDocument{}};
        document.set("integer"_el, BsonValue{int64_t{123456789}});
        document.set("text"_el, BsonValue{"value"_el});
        const auto encoded = document.toByteBlock();
        for (auto length = std::size_t{}; length < encoded.length().toSizeT(); ++length) {
            const auto truncated = encoded.slice(el::unit::ByteIndex::zero(), el::unit::ByteCount::fromSizeT(length));
            REQUIRE_THROWS_AS(el::err::ParseError, BsonValue::fromByteBlockOrThrow(truncated));
            REQUIRE_FALSE(BsonValue::fromByteBlock(truncated).has_value());
        }
    }

    void testTruncatedValueInsideDeclaredDocument() {
        for (
            const auto type :
            {uint8_t{0x01}, uint8_t{0x02}, uint8_t{0x03}, uint8_t{0x05}, uint8_t{0x09}, uint8_t{0x10}, uint8_t{0x12}}) {
            const auto input = bytes({8, 0, 0, 0, type, 'x', 0, 0});
            REQUIRE_THROWS_AS(el::err::ParseError, BsonValue::fromByteBlockOrThrow(input));
        }
    }

    void testDocumentAndWireBytes() {
        auto document = BsonValue{BsonDocument{}};
        document.set(cAField, BsonValue{int32_t{1}});
        const auto encoded = document.toByteBlock();
        REQUIRE_EQUAL(encoded, bytes({12, 0, 0, 0, 0x10, 0x61, 0, 1, 0, 0, 0, 0}));
        const auto decoded = BsonValue::fromByteBlockOrThrow(encoded);
        REQUIRE(decoded.is(BsonType::Document));
        REQUIRE_EQUAL(decoded.getOrThrow(cAField).getInteger().value(), int64_t{1});
        REQUIRE_THROWS_AS(el::err::ParameterFailure, BsonValue{int32_t{1}}.toByteBlock());
    }

    void testKeysRejectNullCharacters() {
        WITH_CONTEXT(requireRejectedKey("\0key"_el));
        WITH_CONTEXT(requireRejectedKey("k\0ey"_el));
        WITH_CONTEXT(requireRejectedKey("key\0"_el));
    }

    void testKeySlicesAndEmbeddedNullValuesRoundTrip() {
        const auto backing = el::text::String{"prefix\0key.suffix\0"_el};
        const auto key = backing.slice({el::unit::ByteIndex{7U}, el::unit::ByteCount{3U}});
        auto document = BsonValue{BsonDocument{}};
        document.set(key, "a\0b"_el).set(""_el, "empty"_el).set(cUnicodeField, "utf8"_el);
        const auto decoded = BsonValue::fromByteBlockOrThrow(document.toByteBlock());
        REQUIRE_EQUAL(decoded.getOrThrow("key"_el).getText().value(), "a\0b"_el);
        REQUIRE_EQUAL(decoded.getOrThrow(""_el).getText().value(), "empty"_el);
        REQUIRE_EQUAL(decoded.getOrThrow(cUnicodeField).getText().value(), "utf8"_el);
    }

    void testNestedAndBinary() {
        auto array = BsonValue{BsonArray{}};
        array.append(BsonValue{true}).append(BsonValue{u8"Yavaş"_el});
        auto document = BsonValue{BsonDocument{}};
        document.set(cPartsField, std::move(array));
        document.set(cRawField, BsonValue{BsonBinary{bytes({1, 2, 3}), 0x80U}});
        const auto decoded = BsonValue::fromByteBlockOrThrow(document.toByteBlock());
        REQUIRE_EQUAL(decoded.getOrThrow(cPartsField).itemCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(decoded.getOrThrow(cRawField).getBinary()->bytes, bytes({1, 2, 3}));
        REQUIRE_EQUAL(decoded.getOrThrow(cRawField).getBinary()->subtype, uint8_t{0x80U});
    }
    void testOpaqueAndMalformed() {
        const auto source = bytes({21, 0, 0, 0, 7, 'i', 'd', 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0});
        const auto value = BsonValue::fromByteBlockOrThrow(source);
        const auto opaque = value.getOrThrow("id"_el).getOpaque();
        REQUIRE(opaque.has_value());
        REQUIRE_EQUAL(opaque->typeCode(), uint8_t{7});
        REQUIRE_EQUAL(value.toByteBlock(), source);
        REQUIRE_FALSE(BsonValue::fromByteBlock(bytes({5, 0, 0, 0, 1})).has_value());
        REQUIRE_FALSE(BsonValue::fromByteBlock(bytes({12, 0, 0, 0, 0x20, 'a', 0, 1, 0, 0, 0, 0})).has_value());
    }
};
