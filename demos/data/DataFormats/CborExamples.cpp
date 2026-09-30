// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DataFormatsDemos.hpp"

#include <DemoCommon.hpp>
#include <erbsland/data/cbor/CborValue.hpp>

#include <vector>

namespace demo {

using namespace el::text::literals;
using el::cbor::CborArray;
using el::cbor::CborFormatOptions;
using el::cbor::CborMap;
using el::cbor::CborParseOptions;
using el::cbor::CborType;
using el::cbor::CborValue;

auto makeExhibition() -> CborValue {
    auto works = CborValue{CborArray{}};
    works.append(CborValue{"Maré"_el}).append(CborValue{"Horizonte"_el});
    auto exhibition = CborValue{CborMap{}};
    exhibition.set("mostra"_el, CborValue{"Esculturas do Mar"_el});
    exhibition.set("obras"_el, std::move(works));
    exhibition.set("ano"_el, CborValue{2026});
    return exhibition;
}

/// Parse a complete CBOR data item and choose how malformed input is reported.
void parseCborBlock() {
    // This block contains the map {"obra": "Lua"}.
    const auto bytes =
        el::ByteBlock::fromVector(std::vector<uint8_t>{0xa1, 0x64, 'o', 'b', 'r', 'a', 0x63, 'L', 'u', 'a'});
    if (const auto parsed = CborValue::fromByteBlock(bytes)) {
        el::io::printLine("Work: "_el, parsed->getOrThrow("obra"_el).getText().value());
    }
    // Ordinary CBOR may use an indefinite array; the writer gives it a definite length.
    const auto indefinite = el::ByteBlock::fromVector(std::vector<uint8_t>{0x9f, 0x01, 0x02, 0xff});
    const auto array = CborValue::fromByteBlockOrThrow(indefinite);
    el::io::printLine("Array items: "_el, array.itemCount().toSizeT());
    el::io::printLine("Rewritten bytes: "_el, array.toByteBlock().length().toRawValue());
    try {
        (void)CborValue::fromByteBlockOrThrow(el::ByteBlock{el::Byte{0xffU}});
    } catch (const el::err::ParseError &) {
        el::io::printLine("Invalid CBOR item"_el);
    }
}

/// Build a CBOR map with an ordered array and native values.
void buildCborValue() {
    const auto exhibition = makeExhibition();
    el::io::printLine("Fields: "_el, exhibition.itemCount().toSizeT());
    el::io::printLine("Works: "_el, exhibition.getOrThrow("obras"_el).itemCount().toSizeT());
}

/// Serialize one value to a binary CBOR block and parse it again.
void serializeCborValue() {
    const auto original = makeExhibition();
    const auto bytes = original.toByteBlock();
    const auto received = CborValue::fromByteBlockOrThrow(bytes);
    el::io::printLine("Encoded bytes: "_el, bytes.length().toRawValue());
    el::io::printLine("Exhibition: "_el, received.getOrThrow("mostra"_el).getText().value());
    const auto positiveSigned = CborValue{int64_t{2026}};
    const auto roundTrip = CborValue::fromByteBlockOrThrow(positiveSigned.toByteBlock());
    el::io::printLine("Positive integer reads as unsigned: "_el, roundTrip.is(CborType::Unsigned));
}

/// Navigate a parsed CBOR tree and replace a nested value without changing the original.
void inspectCborValue() {
    const auto original = CborValue::fromByteBlockOrThrow(makeExhibition().toByteBlock());
    const auto works = original.getOrThrow("obras"_el);
    el::io::printLine("First work: "_el, works.getOrThrow(el::ItemIndex{0U}).getText().value());
    el::io::printLine("Missing is null: "_el, original.get("artista"_el).is(CborType::Null));

    auto revised = original;
    auto revisedWorks = revised.getOrThrow("obras"_el);
    revisedWorks.append(CborValue{"Vento"_el});
    revised.set("obras"_el, revisedWorks);
    el::io::printLine("Original works: "_el, original.getOrThrow("obras"_el).itemCount().toSizeT());
    el::io::printLine("Revised works: "_el, revised.getOrThrow("obras"_el).itemCount().toSizeT());
}

/// Limit the accepted bytes, nesting, values, and text length independently.
void limitCborInput() {
    const auto bytes = makeExhibition().toByteBlock();
    const auto input = CborParseOptions{}.setMaximumInputLength(el::ByteLength{10U});
    const auto nesting = CborParseOptions{}.setMaximumNesting(el::ItemCount{1U});
    const auto values = CborParseOptions{}.setMaximumValueCount(el::ItemCount{3U});
    const auto strings = CborParseOptions{}.setMaximumStringLength(el::ByteLength{4U});
    el::io::printLine("Input accepted: "_el, CborValue::fromByteBlock(bytes, input).has_value());
    el::io::printLine("Nesting accepted: "_el, CborValue::fromByteBlock(bytes, nesting).has_value());
    el::io::printLine("Values accepted: "_el, CborValue::fromByteBlock(bytes, values).has_value());
    el::io::printLine("Text accepted: "_el, CborValue::fromByteBlock(bytes, strings).has_value());
}

/// Write a deterministic DAG-CBOR block and validate incoming bytes with the same profile.
void useDagCbor() {
    const auto exhibition = makeExhibition();
    const auto bytes = exhibition.toByteBlock(CborFormatOptions{}.setDagCbor(true));
    const auto parsed = CborValue::fromByteBlockOrThrow(bytes, CborParseOptions{}.setDagCbor(true));
    el::io::printLine("DAG-CBOR works: "_el, parsed.getOrThrow("obras"_el).itemCount().toSizeT());

    // The integer 1 encoded in two bytes is valid ordinary CBOR, but not DAG-CBOR.
    const auto longOne = el::ByteBlock::fromVector(std::vector<uint8_t>{0x18, 0x01});
    el::io::printLine("Ordinary accepts: "_el, CborValue::fromByteBlock(longOne).has_value());
    el::io::printLine(
        "DAG-CBOR accepts: "_el, CborValue::fromByteBlock(longOne, CborParseOptions{}.setDagCbor(true)).has_value());
}

/// Read CBOR's date/time tag and DAG-CBOR's CID link tag as semantic values.
void readCborTags() {
    // Tag 1 wraps the number 1.5: one and a half seconds after the Unix epoch.
    const auto timeBytes = el::ByteBlock::fromVector(std::vector<uint8_t>{0xc1, 0xfb, 0x3f, 0xf8, 0, 0, 0, 0, 0, 0});
    const auto date = CborValue::fromByteBlockOrThrow(timeBytes);
    el::io::printLine("Date/time value: "_el, date.is(CborType::DateTime));
    el::io::printLine(
        "Written date/time: "_el, CborValue::fromByteBlockOrThrow(date.toByteBlock()).is(CborType::DateTime));

    // Tag 42 wraps a zero prefix and a binary CIDv0 (SHA-256 multihash).
    auto linkWire = std::vector<uint8_t>{0xd8, 0x2a, 0x58, 0x23, 0x00, 0x12, 0x20};
    linkWire.insert(linkWire.end(), 32U, 0U);
    const auto link =
        CborValue::fromByteBlockOrThrow(el::ByteBlock::fromVector(linkWire), CborParseOptions{}.setDagCbor(true));
    el::io::printLine("CID bytes: "_el, link.getLinkBytes()->length().toRawValue());
    const auto rewritten =
        CborValue{el::cbor::CborLink{link.getLinkBytes().value()}}.toByteBlock(CborFormatOptions{}.setDagCbor(true));
    el::io::printLine("Link round trip: "_el, rewritten == el::ByteBlock::fromVector(linkWire));
}

}
