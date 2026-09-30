// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DataFormatsDemos.hpp"

#include <DemoCommon.hpp>
#include <erbsland/data/bson/BsonValue.hpp>
#include <erbsland/err/ParseError.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace demo {

using namespace el::text::literals;
using el::bson::BsonArray;
using el::bson::BsonBinary;
using el::bson::BsonDocument;
using el::bson::BsonParseOptions;
using el::bson::BsonType;
using el::bson::BsonValue;

/// Assemble a plant observation with a document root and an ordered array of readings.
///
/// BSON requires a document at the root. Fields can contain scalars or nested containers, and `append()` adds array
/// values in their wire order.
auto makePlantRecord() -> BsonValue {
    auto readings = BsonValue{BsonArray{}};
    readings.append(BsonValue{int32_t{42}}).append(BsonValue{int32_t{45}});
    auto record = BsonValue{BsonDocument{}};
    record.set("bitki"_el, BsonValue{"lale"_el});
    record.set("nem"_el, std::move(readings));
    record.set("etkin"_el, BsonValue{true});
    return record;
}

/// Parse a complete BSON document received as bytes.
///
/// `fromByteBlock()` is useful when malformed input is an ordinary validation result. The throwing variant reports a
/// `ParseError` when the caller needs to distinguish a rejected document from a valid one.
void parseBsonDocument() {
    // This complete BSON frame contains {"bitki": "lale"}.
    const auto bytes = el::ByteBlock::fromVector(
        std::vector<uint8_t>{21, 0, 0, 0, 2, 'b', 'i', 't', 'k', 'i', 0, 5, 0, 0, 0, 'l', 'a', 'l', 'e', 0, 0});
    const auto parsed = BsonValue::fromByteBlock(bytes);
    if (parsed) {
        el::io::printLine("Plant: "_el, parsed->getOrThrow("bitki"_el).getText().value());
    }

    try {
        (void)BsonValue::fromByteBlockOrThrow(el::ByteBlock{el::Byte{0U}});
    } catch (const el::err::ParseError &) {
        el::io::printLine("Invalid BSON document"_el);
    }
}

/// Build a BSON document from typed fields and an ordered array.
///
/// Start with a document root, append array values, and insert the completed array as a field. A `BsonBinary` retains
/// its subtype, which is needed when the receiving application assigns meaning to binary payloads.
void buildBsonDocument() {
    auto record = makePlantRecord();
    record.set("ornek"_el, BsonValue{BsonBinary{el::ByteBlock{el::Byte{1U}, el::Byte{2U}}, 0x80U}});
    el::io::printLine("Fields: "_el, record.itemCount().toSizeT());
    el::io::printLine("Readings: "_el, record.getOrThrow("nem"_el).itemCount().toSizeT());
    el::io::printLine("Binary subtype: "_el, static_cast<int>(record.getOrThrow("ornek"_el).getBinary()->subtype));
}

/// Serialize a document root into one complete BSON frame.
///
/// `toByteBlock()` produces a `ByteBlock` for transport or storage. It requires a document root, even when the document
/// contains arrays and scalar values below the root.
void serializeBsonDocument() {
    const auto record = makePlantRecord();
    const auto bytes = record.toByteBlock();
    el::io::printLine("Encoded bytes: "_el, bytes.length().toRawValue());
    el::io::printLine("Round-trip fields: "_el, BsonValue::fromByteBlockOrThrow(bytes).itemCount().toSizeT());
}

/// Navigate a parsed document and edit a nested array.
///
/// Child access returns a value. To change a nested child, edit that value and set it back into its parent. The
/// original copy remains unchanged when a copied document is edited.
void inspectBsonDocument() {
    const auto original = BsonValue::fromByteBlockOrThrow(makePlantRecord().toByteBlock());
    const auto readings = original.getOrThrow("nem"_el);
    el::io::printLine("First reading: "_el, readings.getOrThrow(el::ItemIndex{0U}).getInteger().value());
    el::io::printLine("Missing is null: "_el, original.get("konum"_el).is(BsonType::Null));

    auto revised = original;
    auto changedReadings = revised.getOrThrow("nem"_el);
    changedReadings.append(BsonValue{int32_t{47}});
    revised.set("nem"_el, changedReadings);
    el::io::printLine("Original readings: "_el, original.getOrThrow("nem"_el).itemCount().toSizeT());
    el::io::printLine("Revised readings: "_el, revised.getOrThrow("nem"_el).itemCount().toSizeT());
}

/// Apply independent safety limits to BSON input.
///
/// The limits measure complete input bytes, container depth, total fields and array elements, and the UTF-8 byte length
/// of each string or key. A rejected frame returns no value with `fromByteBlock()`.
void limitBsonDocument() {
    const auto bytes = makePlantRecord().toByteBlock();
    const auto input = BsonParseOptions{}.setMaximumInputLength(el::ByteLength{10U});
    const auto nesting = BsonParseOptions{}.setMaximumNesting(el::ItemCount{1U});
    const auto values = BsonParseOptions{}.setMaximumValueCount(el::ItemCount{2U});
    const auto strings = BsonParseOptions{}.setMaximumStringLength(el::ByteLength{3U});
    el::io::printLine("Input accepted: "_el, BsonValue::fromByteBlock(bytes, input).has_value());
    el::io::printLine("Nesting accepted: "_el, BsonValue::fromByteBlock(bytes, nesting).has_value());
    el::io::printLine("Values accepted: "_el, BsonValue::fromByteBlock(bytes, values).has_value());
    el::io::printLine("Strings accepted: "_el, BsonValue::fromByteBlock(bytes, strings).has_value());
}

/// Inspect and preserve a recognized BSON type without interpreting its payload.
///
/// The ObjectId in this example remains an opaque wire value. Its type code and payload can be inspected, and writing
/// the unchanged document preserves the original BSON bytes.
void preserveOpaqueBsonValue() {
    const auto bytes = el::ByteBlock::fromVector(
        std::vector<uint8_t>{21, 0, 0, 0, 7, 'i', 'd', 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0});
    const auto record = BsonValue::fromByteBlockOrThrow(bytes);
    const auto opaque = record.getOrThrow("id"_el).getOpaque().value();
    el::io::printLine("Type code: "_el, static_cast<int>(opaque.typeCode()));
    el::io::printLine("Payload bytes: "_el, opaque.payload().length().toRawValue());
    el::io::printLine("Preserved: "_el, record.toByteBlock() == bytes);
}

}
