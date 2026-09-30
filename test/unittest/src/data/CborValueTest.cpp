// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/data/cbor/CborValue.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <vector>

using namespace el::text::literals;
using namespace el::data::cbor;
TESTED_TARGETS(CborValue CborParseOptions CborFormatOptions)
class CborValueTest final : public el::UnitTest {
private:
    static auto bytes(std::initializer_list<uint8_t> data) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromVector(std::vector<uint8_t>{data});
    }

public:
    void testPrimitiveAndContainers() {
        auto map = CborMap{};
        map.set("tempo"_el, CborValue{int64_t{120}});
        map.set("title"_el, CborValue{u8"Hızlı"_el});
        auto source = CborValue{std::move(map)};
        const auto encoded = source.toByteBlock();
        const auto restored = CborValue::fromByteBlockOrThrow(encoded);
        REQUIRE(restored.is(CborType::Map));
        REQUIRE_EQUAL(restored.getOrThrow("tempo"_el).getSigned().value(), int64_t{120});
        REQUIRE_EQUAL(restored.getOrThrow("title"_el).getText().value(), u8"Hızlı"_el);
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
                bytes({0x63, 'a', 'b', 'c'}), CborParseOptions{}.setMaximumStringLength(el::unit::ByteLength{2U}))
                .has_value());
        REQUIRE_FALSE(
            CborValue::fromByteBlock(bytes({0xd8, 0x2a, 0x41, 0x00}), CborParseOptions{}.setDagCbor(true)).has_value());
    }
    void testTimestampTags() {
        const auto halfSecond = CborValue::fromByteBlockOrThrow(bytes({0xc1, 0xfb, 0x3f, 0xe0, 0, 0, 0, 0, 0, 0}));
        REQUIRE(halfSecond.getDateTime().has_value());
        REQUIRE_EQUAL(halfSecond.getDateTime()->utcTime().nanosecondFraction(), el::time::Nanoseconds{500'000'000});
        REQUIRE_FALSE(CborValue::fromByteBlock(bytes({0xc1, 0xfb, 0x7f, 0xf0, 0, 0, 0, 0, 0, 0})).has_value());
    }
};
