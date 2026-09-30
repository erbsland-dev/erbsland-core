// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/data/bson/BsonValue.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <vector>

using namespace el::text::literals;
using namespace el::data::bson;
TESTED_TARGETS(BsonValue BsonParseOptions BsonFormatOptions)
class BsonValueTest final : public el::UnitTest {
private:
    static auto bytes(std::initializer_list<uint8_t> data) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromVector(std::vector<uint8_t>{data});
    }

public:
    void testDocumentAndWireBytes() {
        auto document = BsonValue{BsonDocument{}};
        document.set("a"_el, BsonValue{int32_t{1}});
        const auto encoded = document.toByteBlock();
        REQUIRE_EQUAL(encoded, bytes({12, 0, 0, 0, 0x10, 0x61, 0, 1, 0, 0, 0, 0}));
        const auto decoded = BsonValue::fromByteBlockOrThrow(encoded);
        REQUIRE(decoded.is(BsonType::Document));
        REQUIRE_EQUAL(decoded.getOrThrow("a"_el).getInteger().value(), int64_t{1});
        REQUIRE_THROWS_AS(el::err::ParameterError, BsonValue{int32_t{1}}.toByteBlock());
    }
    void testNestedAndBinary() {
        auto array = BsonValue{BsonArray{}};
        array.append(BsonValue{true}).append(BsonValue{u8"Yavaş"_el});
        auto document = BsonValue{BsonDocument{}};
        document.set("parts"_el, std::move(array));
        document.set("raw"_el, BsonValue{BsonBinary{bytes({1, 2, 3}), 0x80U}});
        const auto decoded = BsonValue::fromByteBlockOrThrow(document.toByteBlock());
        REQUIRE_EQUAL(decoded.getOrThrow("parts"_el).itemCount(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(decoded.getOrThrow("raw"_el).getBinary()->bytes, bytes({1, 2, 3}));
        REQUIRE_EQUAL(decoded.getOrThrow("raw"_el).getBinary()->subtype, uint8_t{0x80U});
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
