// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cbor/CborValue.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/OutOfRangeError.hpp>
#include <erbsland/OutOfRangeFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <vector>

using namespace el::text::literals;
using namespace el::data::cbor;
TESTED_TARGETS(CborValue CborParseOptions CborFormatOptions)
class CborValueTest final : public el::UnitTest {
private:
    static constexpr auto cEditorField = "editor"_el;
    static constexpr auto cLiteralField = "literal"_el;
    static constexpr auto cMissingField = "missing"_el;
    static constexpr auto cTempoField = "tempo"_el;
    static constexpr auto cTitleField = "title"_el;

private:
    static auto bytes(std::initializer_list<uint8_t> data) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromVector(std::vector<uint8_t>{data});
    }

public:
    void testImplicitTextArguments() {
        CborValue literal = cLiteralField;
        CborValue utf8Literal = u8"Grüße"_el;
        auto editor = el::StringEditor{"before"_el};
        CborValue edited = editor;
        auto object = CborValue{CborMap{}};
        object.set(cLiteralField, "value"_el).set(cEditorField, editor);
        auto array = CborValue{CborArray{}};
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
        const auto object = CborValue{CborMap{}};
        REQUIRE_THROWS_AS(el::err::NotFoundError, object.getOrThrow(cMissingField));
        auto array = CborValue{CborArray{}};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, array.getOrThrow(el::unit::ItemIndex{0U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeFailure, array.set(el::unit::ItemIndex{1U}, CborValue{}));
        REQUIRE_THROWS_AS(el::err::LogicFailure, array.getOrThrow(cMissingField));
    }

    void testTruncatedInputIsAParseFailure() {
        auto document = CborValue{CborMap{}};
        document.set("integer"_el, CborValue{int64_t{123456789}});
        document.set("text"_el, CborValue{"value"_el});
        const auto encoded = document.toByteBlock();
        for (auto length = std::size_t{}; length < encoded.length().toSizeT(); ++length) {
            const auto truncated = encoded.slice(el::unit::ByteIndex::zero(), el::unit::ByteCount::fromSizeT(length));
            REQUIRE_THROWS_AS(el::err::ParseError, CborValue::fromByteBlockOrThrow(truncated));
            REQUIRE_FALSE(CborValue::fromByteBlock(truncated).has_value());
        }
    }

    void testPrimitiveAndContainers() {
        auto map = CborMap{};
        map.set(cTempoField, CborValue{int64_t{120}});
        map.set(cTitleField, CborValue{u8"Hızlı"_el});
        auto source = CborValue{std::move(map)};
        const auto encoded = source.toByteBlock();
        const auto restored = CborValue::fromByteBlockOrThrow(encoded);
        REQUIRE(restored.is(CborType::Map));
        REQUIRE_EQUAL(restored.getOrThrow(cTempoField).getSigned().value(), int64_t{120});
        REQUIRE_EQUAL(restored.getOrThrow(cTitleField).getText().value(), u8"Hızlı"_el);
        auto array = CborValue{CborArray{}};
        array.append(CborValue{true}).append(CborValue{bytes({0, 1, 2})});
        REQUIRE_EQUAL(CborValue::fromByteBlockOrThrow(array.toByteBlock()).itemCount(), el::unit::ItemCount{2U});
    }
    void testDagCanonicalAndRejection() {
        auto map = CborValue{CborMap{}};
        map.set("bb"_el, CborValue{uint64_t{2}}).set("a"_el, CborValue{uint64_t{1}});
        const auto encoded = map.toByteBlock(CborFormatOptions{}.setDagCbor(true));
        REQUIRE_EQUAL(encoded, bytes({0xa2, 0x61, 0x61, 0x01, 0x62, 0x62, 0x62, 0x02}));
        REQUIRE(CborValue::fromByteBlock(encoded, CborParseOptions{}.setDagCbor(true)).has_value());
        REQUIRE_FALSE(CborValue::fromByteBlock(bytes({0x18, 0x01}), CborParseOptions{}.setDagCbor(true)).has_value());
        REQUIRE_FALSE(
            CborValue::fromByteBlock(bytes({0x9f, 0x01, 0xff}), CborParseOptions{}.setDagCbor(true)).has_value());
        REQUIRE_FALSE(
            CborValue::fromByteBlock(bytes({0xfb, 0x7f, 0xf0, 0, 0, 0, 0, 0, 0}), CborParseOptions{}.setDagCbor(true))
                .has_value());
        REQUIRE_FALSE(
            CborValue::fromByteBlock(
                bytes({0xa2, 0x61, 0x62, 0x01, 0x61, 0x61, 0x02}), CborParseOptions{}.setDagCbor(true))
                .has_value());
    }
    void testIndefiniteAndMalformed() {
        const auto value = CborValue::fromByteBlockOrThrow(bytes({0x9f, 0x01, 0x02, 0xff}));
        REQUIRE_EQUAL(value.itemCount(), el::unit::ItemCount{2U});
        REQUIRE_FALSE(CborValue::fromByteBlock(bytes({0xa1, 0x01, 0x02})).has_value());
        REQUIRE_FALSE(CborValue::fromByteBlock(bytes({0x61})).has_value());
        REQUIRE_FALSE(CborValue::fromByteBlock(bytes({0x01, 0x02})).has_value());
        REQUIRE_FALSE(
            CborValue::fromByteBlock(
                bytes({0x63, 'a', 'b', 'c'}), CborParseOptions{}.setMaximumStringLength(el::unit::ByteCount{2U}))
                .has_value());
        REQUIRE_FALSE(
            CborValue::fromByteBlock(bytes({0xd8, 0x2a, 0x41, 0x00}), CborParseOptions{}.setDagCbor(true)).has_value());
    }
    void testTimestampTags() {
        const auto halfSecond = CborValue::fromByteBlockOrThrow(bytes({0xc1, 0xfb, 0x3f, 0xe0, 0, 0, 0, 0, 0, 0}));
        REQUIRE(halfSecond.getTimestamp().has_value());
        REQUIRE_EQUAL(halfSecond.getTimestamp()->time().nanosecondFraction(), el::time::Nanoseconds{500'000'000});
        REQUIRE_FALSE(CborValue::fromByteBlock(bytes({0xc1, 0xfb, 0x7f, 0xf0, 0, 0, 0, 0, 0, 0})).has_value());
    }
};
