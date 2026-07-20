// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/SourceIdentifier.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(SourceIdentifier)
class SourceIdentifierTest final : public el::UnitTest {
public:
    void testCreateAndAccessors() {
        auto fileId = SourceIdentifier::createForFile("config.elcl"_el);
        REQUIRE(fileId->name() == "file"_el);
        REQUIRE(fileId->path() == "config.elcl"_el);

        auto textId = SourceIdentifier::create("text"_el, ""_el);
        REQUIRE(textId->name() == "text"_el);
        REQUIRE(textId->path().isEmpty());
    }

    void testEqualityOperators() {
        auto id1 = SourceIdentifier::createForFile("a.elcl"_el);
        auto id2 = SourceIdentifier::createForFile("a.elcl"_el);
        auto id3 = SourceIdentifier::createForFile("b.elcl"_el);
        auto textId = SourceIdentifier::create("text"_el, ""_el);

        REQUIRE(*id1 == *id2);
        REQUIRE(*id1 != *id3);
        REQUIRE(*id1 != *textId);
    }

    void testToText() {
        auto id = SourceIdentifier::createForFile("path.elcl"_el);
        REQUIRE(id->toText() == "file:path.elcl"_el);

        auto textId = SourceIdentifier::create("text"_el, ""_el);
        REQUIRE(textId->toText() == "text"_el);

        auto idNoName = SourceIdentifier::create(""_el, "path.elcl"_el);
        REQUIRE(idNoName->toText() == "unknown:path.elcl"_el);

        auto idNoPath = SourceIdentifier::createForFile(""_el);
        REQUIRE(idNoPath->toText() == "file"_el);
    }

    void testAreEqualHelper() {
        auto id1 = SourceIdentifier::createForFile("a.elcl"_el);
        auto id2 = SourceIdentifier::createForFile("a.elcl"_el);
        auto textId = SourceIdentifier::create("text"_el, ""_el);
        REQUIRE(SourceIdentifier::areEqual(id1, id2));
        REQUIRE(SourceIdentifier::areEqual(SourceIdentifierPtr{}, SourceIdentifierPtr{}));
        REQUIRE_FALSE(SourceIdentifier::areEqual(id1, SourceIdentifierPtr{}));
        REQUIRE_FALSE(SourceIdentifier::areEqual(id1, textId));
    }
};
