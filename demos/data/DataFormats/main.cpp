// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DataFormatsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ParseBsonDocument"_el, parseBsonDocument);
    app.registerDemo("BuildBsonDocument"_el, buildBsonDocument);
    app.registerDemo("SerializeBsonDocument"_el, serializeBsonDocument);
    app.registerDemo("InspectBsonDocument"_el, inspectBsonDocument);
    app.registerDemo("LimitBsonDocument"_el, limitBsonDocument);
    app.registerDemo("PreserveOpaqueBsonValue"_el, preserveOpaqueBsonValue);
    app.registerDemo("ParseCborBlock"_el, parseCborBlock);
    app.registerDemo("BuildCborValue"_el, buildCborValue);
    app.registerDemo("SerializeCborValue"_el, serializeCborValue);
    app.registerDemo("InspectCborValue"_el, inspectCborValue);
    app.registerDemo("LimitCborInput"_el, limitCborInput);
    app.registerDemo("UseDagCbor"_el, useDagCbor);
    app.registerDemo("ReadCborTags"_el, readCborTags);
    app.registerDemo("BsonTempoVariation"_el, bsonTempoVariation);
    app.registerDemo("CborTempoVariation"_el, cborTempoVariation);
    app.registerDemo("XmlTempoVariation"_el, xmlTempoVariation);
    app.registerDemo("ParseXmlDocument"_el, parseXmlDocument);
    app.registerDemo("BuildXmlDocument"_el, buildXmlDocument);
    app.registerDemo("BuildXmlMarkup"_el, buildXmlMarkup);
    app.registerDemo("SerializeXmlDocument"_el, serializeXmlDocument);
    app.registerDemo("InspectXmlDocument"_el, inspectXmlDocument);
    app.registerDemo("InspectXmlMarkup"_el, inspectXmlMarkup);
    app.registerDemo("LimitXmlInput"_el, limitXmlInput);
    return app.run();
}
