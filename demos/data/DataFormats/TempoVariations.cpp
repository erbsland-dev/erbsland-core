// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DataFormatsDemos.hpp"

#include <DemoCommon.hpp>
#include <erbsland/data/bson/BsonValue.hpp>
#include <erbsland/data/cbor/CborValue.hpp>
#include <erbsland/data/xml/XmlDocument.hpp>

namespace demo {

using namespace el::text::literals;

/// Write and read a Turkish tempo variation as a BSON document.
void bsonTempoVariation() {
    auto document = el::bson::BsonValue{el::bson::BsonDocument{}};
    document.set("ritim"_el, el::bson::BsonValue{"aksak"_el});
    document.set("bpm"_el, el::bson::BsonValue{int32_t{96}});
    const auto received = el::bson::BsonValue::fromByteBlockOrThrow(document.toByteBlock());
    el::io::printLine(
        "BSON: "_el,
        received.getOrThrow("ritim"_el).getText().value(),
        " at "_el,
        received.getOrThrow("bpm"_el).getInteger().value(),
        " bpm"_el);
}

/// Write and read the same variation as a CBOR map.
void cborTempoVariation() {
    auto map = el::cbor::CborValue{el::cbor::CborMap{}};
    map.set("ritim"_el, el::cbor::CborValue{"aksak"_el});
    map.set("bpm"_el, el::cbor::CborValue{int64_t{96}});
    const auto wire = map.toByteBlock(el::cbor::CborFormatOptions{}.setDagCbor(true));
    const auto received =
        el::cbor::CborValue::fromByteBlockOrThrow(wire, el::cbor::CborParseOptions{}.setDagCbor(true));
    el::io::printLine(
        "DAG-CBOR: "_el,
        received.getOrThrow("ritim"_el).getText().value(),
        " at "_el,
        received.getOrThrow("bpm"_el).getSigned().value(),
        " bpm"_el);
}

/// Build, serialize, and parse a small XML tempo variation document.
void xmlTempoVariation() {
    auto document = el::xml::XmlDocument{};
    auto root = document.addRoot("ritimler"_el);
    auto rhythm = root->addElement("ritim"_el, "aksak"_el);
    rhythm->setAttribute("bpm"_el, "96"_el);
    const auto received = el::xml::XmlDocument::fromStringOrThrow(document.toString());
    const auto item = received.root()->children().get(el::ItemIndex{});
    el::io::printLine(
        "XML: "_el, item->textContentOrThrow(), " at "_el, item->getAttribute("bpm"_el).value(), " bpm"_el);
}

}
